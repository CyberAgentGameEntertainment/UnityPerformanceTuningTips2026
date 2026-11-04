# **RenderGraphのUnsafePassを使うべき場面**

名前：チャン ユービン  
ローマ字：ZHANG YUBING  
所属：株式会社サイバーエージェント SGEコア技術本部

# **はじめに**

Unity 6では、Render Graphによるレンダリングパイプラインの構築時に、処理内容に応じてRasterRenderPass、ComputePass、UnsafePassを使い分けます。本章では、UnsafePassを使うべき場面を紹介します。

Render Graphには依存関係の解析やSubPassマージといった最適化があります。そのため、通常の描画にはRasterRenderPassを、Compute Shaderを使う処理にはComputePassを優先するのが基本です。

UnsafePassを選ぶべき場面は、主に次の4つです。

* RasterRenderPassやComputePassでは実現できない処理がある場合  
* CommandBuffer自身が必要な場合  
* Compatibility ModeからRender Graphへ素早く移行したい場合  
* 1つの描画処理が多くマージできないPassで構成されている場合

最初の3つは、UnsafePassを使用しなければ実現できない、または実装コストが大きい場合です。最後の1つは、必須ではないものの、Passを1つのUnsafePassにまとめることでCPU負荷の低減とRender Graph Viewerの見通しの改善が見込める場合です。

これから、これらの場面を順に解説していきます。

# **UnsafePassを選ぶべき場面**

UnsafePassはC\#のunsafe機能を意味するものではありません。Render Graphによる一部の自動管理や検証を抑え、CommandBufferに近いAPIを利用できるようにしたPassです。

## **RasterRenderPassやComputePassでは実現できない処理がある場合**

RasterRenderPassやComputePassで使用するRasterCommandBuffer／ComputeCommandBufferは、CommandBufferのうち各Passの役割に必要なAPIだけを公開したラッパーであり、以下のようなAPIは利用できません。

* SetRenderTarget：Load/Store Actionを指定しながらレンダーターゲット、MRT、ミップ、配列スライスを細かく指定する  
* CopyTexture：ミップ、配列要素、矩形領域を指定してテクスチャをコピーする  
* GenerateMips：レンダーターゲットのミップマップを生成する  
* SetRandomWriteTarget/ClearRandomWriteTargets：ピクセルシェーダーのランダム書き込みターゲットを設定または解除する  
* RequestAsyncReadback：GPUリソースの非同期読み戻し要求を記録する

これらのAPIは、UnsafePassのUnsafeCommandBufferで利用できます。さらに、CommandBufferHelpers.GetNativeCommandBufferでネイティブのCommandBufferを取得すれば、より高度なAPIも利用できるようになります。

## **CommandBuffer自身が必要な場合**

エンジンのコールバック、IMGUI、SDKの描画処理など、外部コードがCommandBufferを受け取って任意のコマンドを記録する処理では、記録される内容が実行時まで分かりません。記録時点で処理内容を宣言するRasterRenderPassやComputePassには、このようなブラックボックス処理を組み込めません。UnsafePassでネイティブのCommandBufferを取得し、外部コードへ渡すことで、Render Graphに組み込めます。

## **Compatibility ModeからRender Graphへ素早く移行したい場合**

Compatibility ModeでExecuteメソッドに記述していたCommandBufferの処理は、コマンド列をほぼそのままUnsafePassのSetRenderFunc内へ移せます。そのため、少ない工数でRender Graphへの移行と動作確認を進めたい場合に有効です。

ただし、これはあくまで素早く移行するための暫定的な手段です。時間を確保できる場合は、処理内容に応じてRasterRenderPassやComputePassへ分類して移植し、UnsafePassは宣言的なAPIで表現できない部分だけに限定することを推奨します。

## **多くマージできないPassで構成されている場合**

連続してマージできないPassを1つのUnsafePassにまとめることでCPU負荷の低減とRender Graph Viewerの見通しを改善する効果があります。

### **典型的なケース**

* Ping-pong方式のBlur  
* BloomのMip Pyramid生成  
* 複数回のダウンサンプルとアップサンプル  
* 前の処理の出力を次の処理でサンプリングするフィルタ処理

これらの処理では、Pass毎に描画ターゲットを切り替えたり、直前Passの出力を周辺サンプリングしたりすることで、SubPassマージが成立しません。

### **UnsafePassにまとめるとCPU負荷が下がる**

RasterRenderPassを1つ登録するたびに、次の処理が発生します。

* Passの登録とPassDataの準備  
* UseTextureやSetRenderAttachmentによるリソース依存関係の登録  
* Passの依存関係解析とマージ候補の確認  
* Profilingスコープの開始と終了  
* レンダーターゲットの設定と実行コールバックの呼び出し

RasterRenderPassが増えるほど、CPU側のコストも増加します。マージできないPassチェーンをRasterRenderPassとして登録しても、マージの恩恵を得られないまま、登録や管理のコストが積み重なります。一方、それらを1つのUnsafePassにまとめると、Render Graph上では1つのPassとして登録されます。描画コマンドやレンダーターゲット切り替えのコストは変わりませんが、Render GraphがPass単位で行う登録/管理のコストを削減できます（{{fig:unsafe_pass_pass_chain}}）。

![][image1]  
図: マージできないPassチェーンにおけるRender Graph上のPass数と描画回数 {{fig:unsafe_pass_pass_chain}}

### **CPU負荷軽減効果の測定例**

本プロジェクトで実施したEditor上の測定では、256×256のRender Textureに対する4-tapのPing-pong Blurについて、Blur処理ごとにRasterRenderPassを登録する方式と、1つのUnsafePass内で一連のBlur処理を実行する方式を比較しました。測定条件は、Apple M4 Max（macOS 26.5）、Unity 6000.3.19f1、URP 17.3です。各組み合わせで500フレームを採取し、切り替え後の60フレームを除外した中央値を使用しました。測定値は、Render Graphの記録、ハッシュ計算、コンパイル、実行に関するCPU markerの合計です。

表: Ping-pong BlurにおけるRender Graph関連CPU時間の比較 {{table:unsafe_pass_pingpong_cpu}}

| Blur回数 | RasterRenderPass×N（µs） | UnsafePass×1（µs） | 差分（µs） |
| :---- | :---- | :---- | :---- |
| 5 | 286.4 | 280.0 | 6.4 |
| 10 | 334.8 | 313.6 | 21.2 |
| 20 | 427.8 | 389.1 | 38.7 |
| 50 | 675.9 | 592.9 | 83.0 |
| 100 | 1080.7 | 898.5 | 182.2 |

測定値はEditor環境における傾向確認用の値です。Blur回数が増えるほど、RasterRenderPass方式とUnsafePass方式の差が拡大しています。線形近似では、両方式の傾きの差、すなわちRasterRenderPassの追加固定コストは1Passあたり約1.8µsでした。この値はEditorでの測定値であり、実機での絶対値を示すものではありませんが、Blur回数に応じてCPU負荷の差が増えることを確認できます。  
この効果は、Render Graphの記録、実行、Profilingスコープ、レンダーターゲット設定など、Pass単位の登録/管理コストが削減されることによるものです。描画コマンドに差がないので、UnsafePass化によって描画負荷が減るわけではありません。

### **Render Graph Viewerの見通しも改善する**

RasterRenderPassを多く登録すると、Render Graph Viewerには登録した数だけPassが表示されます。グラフが横方向に長くなるため、機能ごとの区切りが見づらくなり、リソースの依存関係も把握しにくくなります（{{fig:raster_pass_viewer}}）。

![][image2]  
図: Blur処理ごとにRasterRenderPassを登録した場合のRender Graph Viewer表示 {{fig:raster_pass_viewer}}

機能内部のPassチェーンを1つのUnsafePassにまとめると、Viewerでは機能全体を表す1つのPassとして確認できます（{{fig:unsafe_pass_viewer}}）。これにより、機能ごとの区切りが明確になり、リソースの依存関係も把握しやすくなります。![][image3]

図: 一連のBlur処理を1つのUnsafePassにまとめた場合のRender Graph Viewer表示 {{fig:unsafe_pass_viewer}}

CPU負荷の削減に加えて、プロファイリング時のノイズを減らせる点も大きな利点です。

ただし、Render Graph Viewerの見通しを改善するためだけに、異なる機能の処理やSubPassマージが成立する処理を無理にUnsafePassにまとめることは推奨されません。

# **UnsafePass使用時の注意事項**

## **レンダリングターゲットの設定**

RasterRenderPassでは記録時にSetRenderAttachmentでレンダーターゲットを宣言しますが、UnsafePassでは実行時にSetRenderTargetを呼び出して設定します。

## **依存関係を宣言する**

UnsafePassを使用してもRender Graphから完全に切り離されるわけではありません。使用するテクスチャは、必ずUseTextureで読み書きの依存関係を宣言する必要があります。宣言が不足すると、リソースのライフタイム管理や同期が正しく行われません。

RasterRenderPassでは、レンダーターゲットの使用はSetRenderAttachmentで宣言されますが、UnsafePassではSetRenderTargetの呼び出しだけでは依存関係が登録されません。レンダーターゲットも必ずUseTextureで読み書きの依存関係を宣言する必要があります。

## **Load/Store Actionの設定**

RenderBufferLoadActionはレンダーターゲットを有効にするときの既存内容の扱い、RenderBufferStoreActionは描画完了後の結果の扱いを指定します。Tile-Based GPUでは、不要なLoadやStoreがタイルメモリとメインメモリの転送を増やすため、適切な設定が重要です。

RasterRenderPassでは、宣言したAccessFlagsをもとにRender GraphがLoad/Store Actionを自動的に決定します。一方、UnsafePassでSetRenderTarget(target)のようにActionを省略すると、既定動作であるLoad／Storeが適用され、不要なメモリ転送によって性能が低下する可能性があります。

そのため、UnsafePassではSetRenderTargetの呼び出し時に、必ずLoad/Store Actionを明示します。

* Load側：全画面を上書きするならDontCare、既存内容を読む場合だけLoad  
* Store側：結果を後続の処理で読むならStore、どこからも使わない場合だけDontCare

# **コード例：Ping-pong BlurのマージできないPassチェーン**

本節に実際のコード例を紹介します。なお、掲載するコードは要点を示すための抜粋であり、そのまま動作する完全なコードではありません。

2枚のテクスチャを交互に読み書きしながらBlurを連続して適用する例です。同じ処理をRasterRenderPassで実装した場合を示し、その後に1つのUnsafePassへまとめた場合を示します。descはBlur用のRender Texture記述子、blurMaterialはBlur用のマテリアルです。

## **RasterRenderPass方式：Blur処理ごとにPassを登録する**

RasterRenderPass方式では、{{list:raster_pass_pingpong_blur}} のように、ループ内でBlur処理ごとにAddRasterRenderPassを呼び出します。各Passの入力と出力は明確に宣言できますが、Blur回数が増えるほどRender Graph上のPass数も増加します。

`CODE START csharp | RasterRenderPassでPing-pong Blurを実装する {{list:raster_pass_pingpong_blur}}`  
`class BlurRasterPassData`  
`{`  
    `internal TextureHandle source;`  
    `internal TextureHandle target;`  
    `internal Material material;`  
`}`

`var ping = renderGraph.CreateTexture(desc);`  
`var pong = renderGraph.CreateTexture(desc);`  
`var current = resourceData.activeColorTexture;`  
`var writePing = true;`

`for (var i = 0; i < blurIterations; i++)`  
`{`  
    `var target = writePing ? ping : pong;`

    `using (var builder = renderGraph.AddRasterRenderPass<BlurRasterPassData>(`  
        `"Ping-pong Blur Raster", out var passData))`  
    `{`  
        `passData.source = current;`  
        `passData.target = target;`  
        `passData.material = blurMaterial;`

        `builder.UseTexture(passData.source, AccessFlags.Read);`  
        `builder.SetRenderAttachment(passData.target, 0, AccessFlags.Write);`  
        `builder.SetRenderFunc((BlurRasterPassData data, RasterGraphContext context) =>`  
        `{`  
            `Blitter.BlitTexture(`  
                `context.cmd, data.source, new Vector4(1, 1, 0, 0),`  
                `data.material, 0);`  
        `});`  
    `}`

    `current = target;`  
    `writePing = !writePing;`  
`}`  
`CODE END`

## **UnsafePass方式：1つのPass内で一連の処理を実行する**

UnsafePass方式では、{{list:unsafe_pass_pingpong_blur}} のように、Render Graphへの登録を1回にして、SetRenderFunc内でレンダーターゲットを切り替えます。Blur処理ごとの描画コマンドは残りますが、Pass単位の管理を1回にまとめられます。

`CODE START csharp | Ping-pong Blurを1つのUnsafePassにまとめる {{list:unsafe_pass_pingpong_blur}}`  
`class PingPongBlurPassData`  
`{`  
    `internal TextureHandle source;`  
    `internal TextureHandle ping;`  
    `internal TextureHandle pong;`  
    `internal int iterations;`  
    `internal Material material;`  
`}`

`var ping = renderGraph.CreateTexture(desc);`  
`var pong = renderGraph.CreateTexture(desc);`  
`var output = blurIterations == 0`  
    `? resourceData.activeColorTexture`  
    `: blurIterations % 2 == 0 ? ping : pong;`

`using (var builder = renderGraph.AddUnsafePass<PingPongBlurPassData>(`  
    `"Ping-pong Blur", out var passData))`  
`{`  
    `passData.source = resourceData.activeColorTexture;`  
    `passData.ping = ping;`  
    `passData.pong = pong;`  
    `passData.iterations = blurIterations;`  
    `passData.material = blurMaterial;`

    `builder.UseTexture(passData.source, AccessFlags.Read);`  
    `builder.UseTexture(passData.ping, AccessFlags.ReadWrite);`  
    `builder.UseTexture(passData.pong, AccessFlags.ReadWrite);`  
    `builder.SetRenderFunc((PingPongBlurPassData data, UnsafeGraphContext context) =>`  
    `{`  
        `var cmd = CommandBufferHelpers.GetNativeCommandBuffer(context.cmd);`  
        `var current = data.source;`  
        `var next = data.pong;`  
        `var alternate = data.ping;`  
        `for (var i = 0; i < data.iterations; i++)`  
        `{`  
            `cmd.SetRenderTarget(`  
                `next,`  
                `RenderBufferLoadAction.DontCare,`  
                `RenderBufferStoreAction.Store);`  
            `Blitter.BlitTexture(`  
                `cmd, current, new Vector4(1, 1, 0, 0),`  
                `data.material, 0);`

            `current = next;`  
            `(next, alternate) = (alternate, next);`  
        `}`  
    `});`  
`}`  
`CODE END`

# **まとめ**

本章では、Render GraphでUnsafePassを使うべき場面と、使用時の注意事項を紹介しました。

UnsafePassを選ぶべき場面は次の4つです。

* RasterCommandBuffer/ComputeCommandBufferに公開されていないAPIが必要な場合  
* 外部コードへ渡すCommandBufferそのものが必要な場合  
* Compatibility ModeからRender Graphへ素早く移行したい場合  
* マージできないPassチェーンを1つのPassにまとめたい場合

マージできないPassチェーンを1つのUnsafePassにまとめると、Pass単位の登録/管理コストが削減され、Render Graph Viewerの見通しも改善します。本プロジェクトのEditor上の測定（{{table:unsafe_pass_pingpong_cpu}}）では、RasterRenderPass1つあたり約1.8µsの固定コストを削減できました。ただし、描画コマンド自体は変わらないため、GPU負荷の削減を目的とした手法ではない点に注意が必要です。

一方で、UnsafePassはRender Graphによる自動管理や検証の一部を手放すPassでもあります。使用する際は、UseTextureによる依存関係の宣言と、SetRenderTargetでのLoad/Store Actionの明示を忘れないようにしてください。

基本はRasterRenderPassとComputePassを優先し、UnsafePassは宣言的なAPIで表現できない処理や、マージできないPassチェーンの整理に限定して使うことで、Render Graphの最適化とCommandBufferの自由度を両立できます。

# **参考資料**

* [Unity Manual \- Use the CommandBuffer interface in a render graph (UnsafePass)](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/render-graph-unsafe-pass.html)  
* [Unity Manual \- Optimize a render graph](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/render-graph-optimize.html)  
* [Unity Scripting API \- CommandBuffer](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.CommandBuffer.html)  
* [Unity Scripting API \- RenderBufferLoadAction](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.RenderBufferLoadAction.html)  
* [Unity Scripting API \- RenderBufferStoreAction](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.RenderBufferStoreAction.html)

[image1]: assets/zhang-yubing-1/media/image1.png

[image2]: assets/zhang-yubing-1/media/image2.png

[image3]: assets/zhang-yubing-1/media/image3.png