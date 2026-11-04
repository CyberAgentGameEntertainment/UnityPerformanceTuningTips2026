# **URPのFinalBlitPassを省略する方法**

名前：チャン ユービン  
ローマ字：ZHANG YUBING  
所属：株式会社サイバーエージェント SGEコア技術本部

# **FinalBlitPassとは**

URPのレンダリングパイプラインにおいて、実際に画面へ表示されるのはBackBufferです。  
しかしBackBufferは書き込み専用で、読み込むことができません。そのためポストプロセスのように途中結果を入力として読む処理がある場合、カメラはいったん中間テクスチャへ描画され、最後にBackBufferへBlitされる仕組みになっています。  
この中間テクスチャからBackBufferへのBlitを担当するのが、URPのFinalBlitPassです。  
ただのコピーではなく、画面へ出す直前に必要な最終処理もまとめて担当しています。

* カメラのViewportを考慮した転送  
* Linear→sRGB変換  
* HDR Output時のエンコードとオーバーレイUIの合成  
* UV原点が異なる場合のY反転  
* スケーリング時のフィルタ切り替え

## **FinalBlitの機能を持つ3種類のFinalBlitPass**

「中間テクスチャをBackBufferへ出力する」役割を持つパスは、実はURPに3種類あり、条件によってどれか1つだけが選ばれます。

### **1\. UberPost**

ポストプロセスが有効であれば追加されるパスですが、以下の条件を満たす場合にFinalBlitの機能も兼ねます。

* カメラスタックの最後のカメラであること  
* 後述のFinalPostが不要であること  
* AfterRenderingPostProcessing以降に実行されるカスタムパスが無いこと

### **2\. FinalPost**

ポストプロセスが有効で、かつUberPostでは処理しきれない「最終工程」がある場合に追加されるパスです。

1. FXAA  
2. Linear以外のアップスケール … Point / FSR / STPなど。  
3. TAA \+ CAS … RCASシャープニングを独立パスで実行する必要がある

### **3\. FinalBlitPass**

上2つのどちらもBackBufferを書いていない場合の、最後の砦です。実際に出るのは主に次のケースです。

* ポストプロセスが無効で、かつ中間テクスチャが存在する  
* ポストプロセスは有効だが、その後ろにカスタムパスがある  
* ポストプロセスは有効だが、Camera.AddCommandBufferAsyncなどのキャプチャ処理がある

### **3種類の関係を整理**

表: 最終出力を担当するパスと条件 {{table:final_blit_pass_selection}}

| 最終出力を担当するパス | 条件 |
| :---- | :---- |
| UberPost | ポストプロセス有効、FinalPost不要、後続のカスタムパスもキャプチャも無し |
| FinalPost | ポストプロセス有効、かつFXAA / Linear以外のアップスケール / TAA \+ CAS のいずれか |
| FinalBlitPass | 上2つがBackBufferを書いておらず、中間テクスチャがある場合 |
| いずれも無し | 中間テクスチャがない、またはカメラスタック上の最後のカメラではない |

# **FinalBlitPassを省略する方法**

URPのポストプロセスを使う時や通常のカスタムポストプロセスを作る時もFinalBlitPassを気にする必要がありません。  
ただし「最終工程」に相当するカスタムパスを入れた場合、URPのFinalBlitPassは余計なBlitを1回挟むだけの無駄なパスになってしまいます。  
そこでUnity 6.3から、カスタムパス側から「今からアクティブターゲットがBackBufferになる」と宣言し、URPのFinalBlitPassを省略できるようになりました。  
`CODE START csharp | BackBufferへの切り替え {{list:switch_active_textures_to_backbuffer}}`  
`var resourceData = frameData.Get<UniversalResourceData>();`  
`resourceData.SwitchActiveTexturesToBackbuffer();`  
`CODE END`  
実装は単純です（Runtime/FrameData/UniversalResourceData.cs）。  
`CODE START csharp | SwitchActiveTexturesToBackbufferの実装 {{list:switch_active_textures_implementation}}`  
`public void SwitchActiveTexturesToBackbuffer()`  
`{`  
    `activeColorID = UniversalResourceData.ActiveID.BackBuffer;`  
    `activeDepthID = UniversalResourceData.ActiveID.BackBuffer;`  
`}`  
`CODE END`  
frameData上のIDを2つ書き換えるだけで、BlitもコピーもRenderPassの追加も一切行いません。このAPIは「転送処理」ではなく、URPに対する宣言です。activeColorTexture / activeDepthTextureはこのIDを見て返すTextureHandleを切り替えるだけのswitchになっています。  
`CODE START csharp | activeColorTextureの切り替え {{list:active_color_texture_switch}}`  
`public TextureHandle activeColorTexture`  
`{`  
    `get`  
    `{`  
        `switch (activeColorID)`  
        `{`  
            `case ActiveID.Camera:     return cameraColor;`  
            `case ActiveID.BackBuffer: return backBufferColor;`  
            `...`  
        `}`  
    `}`  
`}`  
`CODE END`  
SwitchActiveTexturesToBackbuffer()実行後、URPのFinalBlitPassは生成されなくなります。

## **使用例：カスタムFXAA**

カスタムFXAAポストプロセスを例に、前後の比較をします。  
かつての典型的なフローは3回のBlitが必要でした。  
`CODE START text | 従来のFXAAフロー {{list:custom_fxaa_legacy_flow}}`  
`CameraColor → [FXAA] → FXAABuffer → [Blit] → CameraColor → [URP FinalBlit] → BackBuffer`  
`CODE END`  
Unity 6.0以降はCameraColorを直接差し替えられるので2回に減りますが、FXAABufferという中間テクスチャは必要で、FinalBlitPassも引き続き作られます。  
`CODE START text | Unity 6.0のFXAAフロー {{list:custom_fxaa_unity6_flow}}`  
`CameraColor → [FXAA] → FXAABuffer → [URP FinalBlit] → BackBuffer`  
`CODE END`  
Unity 6.3でBackBuffer切り替えを使うと、1回で済みます。中間テクスチャも不要です。  
`CODE START text | Unity 6.3のFXAAフロー {{list:custom_fxaa_direct_backbuffer_flow}}`  
`CameraColor → [FXAA] → BackBuffer`  
`CODE END`  
コードは次のようになります。なお以降のコードは、要点を示すために一部を抜粋したものです。そのままでは動作しません。  
{{pagebreak}}
`CODE START csharp | BackBufferへ直接出力するカスタムFXAA {{list:custom_fxaa_direct_backbuffer}}`  
`private class PassData`  
`{`  
    `public TextureHandle Source;`  
    `public TextureHandle Target;`  
    `public Material FXAAMaterial;`  
`}`

`public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)`  
`{`  
    `var cameraData = frameData.Get<UniversalCameraData>();`  
    `var resourceData = frameData.Get<UniversalResourceData>();`

    `// 切り替え前にCameraColorを取得しておく`  
    `var cameraColor = resourceData.activeColorTexture;`

    `// 以降のactiveColorTextureはBackBufferColorを返す`  
    `// これでURPのFinalBlitPassが生成されなくなる`  
    `resourceData.SwitchActiveTexturesToBackbuffer();`  
    `var backBufferColor = resourceData.activeColorTexture;`

    `// LinearToSRGBConversionが必要かどうかの判定`  
    `var requireSrgbConversion = cameraData.targetTexture == null`  
                                `&& Display.main.requiresSrgbBlitToBackbuffer;`  
    `CoreUtils.SetKeyword(_fxaaMaterial, ShaderKeywordStrings.LinearToSRGBConversion, requireSrgbConversion);`

    `using (var builder = renderGraph.AddRasterRenderPass<PassData>("Custom FXAA Pass", out var passData))`  
    `{`  
        `passData.Source = cameraColor;`  
        `passData.Target = backBufferColor;`  
        `passData.FXAAMaterial = _fxaaMaterial;`  
        `builder.SetRenderAttachment(backBufferColor, 0, AccessFlags.WriteAll);`  
        `builder.UseTexture(passData.Source, AccessFlags.Read);`  
        `builder.SetRenderFunc(static (PassData data, RasterGraphContext context) =>`  
        `{`  
            `// SourceとTargetのUV原点が異なる場合のみY反転が必要`  
            `var yFlip = context.GetTextureUVOrigin(data.Source) != context.GetTextureUVOrigin(data.Target);`  
            `var scaleBias = yFlip ? new Vector4(1, -1, 0, 1) : new Vector4(1, 1, 0, 0);`  
            `Blitter.BlitTexture(context.cmd, data.Source, scaleBias, data.FXAAMaterial, 0);`  
        `});`  
    `}`  
`}`  
`CODE END`  
Unity 6.0ではFinalBlitPassが常に作成されていました。  
![][image1]  
Unity 6.3でBackBuffer切り替えを使うと、1回のBlitでFXAA処理が完了しBackBufferへ直接出力されます。  
![][image2]

# **URPのFinalBlitを省略する時の注意事項**

SwitchActiveTexturesToBackbuffer()は内部のテクスチャIDを書き換えるだけのAPIです。裏を返せば、辻褄を合わせる責任はすべて呼び出し側にあります。

## **カメラの最後にだけSwitchActiveTexturesToBackbuffer()を呼び出すこと**

このAPIを呼んだ瞬間から、以降のすべてのパスのactiveColorTexture / activeDepthTextureはBackBufferを指します。しかしBackBufferはReadできないため、activeColorTexture / activeDepthTextureを読み込む後続処理はすべて破綻します。  
またcameraData.resolveFinalTarget \== false（後続のカメラがある）場合も切り替えてはいけません。OverlayカメラはCameraColorに積み重ねて描画する設計のため、途中のカメラがBackBufferへ直接書くと後続カメラの描画結果と混ざるか上書きされて消えます。  
したがって呼び出しは、最後のカメラの、最終出力を行うカスタムパスの直前に限定してください。

## **最後に実行するカスタムパスにFinalBlitの機能を実装すること**

FinalBlitPassを省略するということは、FinalBlitPassが担っていた最終処理も一緒に消えるということです。カスタムパス側で肩代わりする必要があります。

### **Y反転**

SourceとTargetのUV原点が一致しない場合、反転しないと画面が上下逆になります。Unity 6.3で追加されたGetTextureUVOrigin()で両者を比較して判定します。このAPIはRasterGraphContextなど実行時のGraphContextのみが持つため、RecordRenderGraphの記録時ではなくSetRenderFunc内で呼ぶ必要があります。  
`CODE START csharp | Y反転の判定 {{list:y_flip_detection}}`  
`var yFlip = context.GetTextureUVOrigin(data.Source) != context.GetTextureUVOrigin(data.Target);`  
`var scaleBias = yFlip ? new Vector4(1, -1, 0, 1) : new Vector4(1, 1, 0, 0);`  
`CODE END`  
なおSystemInfo.graphicsUVStartsAtTopはグラフィックスAPIのUV規約を示すだけで、「今回のBlitで反転が必要か」の判定には使えません。

### **sRGB変換**

cameraData.requireSrgbConversionを見てLinearToSRGBConversionキーワードを設定する必要があり、Shader側でそのキーワードに応じた変換処理を実装する必要があります。  
`CODE START csharp | sRGB変換キーワードの設定 {{list:srgb_conversion_keyword}}`  
`var requireSrgbConversion = cameraData.targetTexture == null && Display.main.requiresSrgbBlitToBackbuffer;`  
`cmd.SetKeyword(ShaderGlobalKeywords.LinearToSRGBConversion, data.requireSrgbConversion);`  
`CODE END`  
`CODE START hlsl | sRGB変換 {{list:srgb_conversion_shader}}`  
`#ifdef _LINEAR_TO_SRGB_CONVERSION`  
    `color = LinearToSRGB(color);`  
`#endif`  
`CODE END`

### **Viewport設定**

Viewportは通常全画面に設定されているが、全画面以外に設定したい場合、CameraのViewportを設定する必要があります。  
`CODE START csharp | Viewportの設定 {{list:viewport_setup}}`  
`var targetViewPort = new Rect(0, 0, targetWidth, targetHeight);`  
`cmd.SetViewport(targetViewPort);`  
`CODE END`

### **HDR OutputとオーバーレイUI合成**

HDRディスプレイ出力時のエンコードや、cameraData.rendersOverlayUIがtrueのときのオーバーレイUI合成もFinalBlitPassの仕事でした。HDR出力対応が必要なタイトルでは、HDROutputUtilsを使って同等の処理を実装する必要があります。

要件が多いように見えますが、プロジェクトで実際に使う機能だけ実装すれば十分です。SDR固定・Viewport全画面・オーバーレイUI未使用のタイトルであれば、実質必要なのはY反転とsRGB変換だけになります。

# **まとめ**

* URPでBackBufferへの最終出力を担当するパスはUberPost / FinalPost / FinalBlitPassの3種類あり、条件によってどれか1つだけが選ばれる  
* カスタムパスをAfterRenderingPostProcessing以降に注入するとhasPassesAfterPostProcessingがtrueになり、FinalBlitPassがほぼ必ず追加される  
* Unity 6.3のSwitchActiveTexturesToBackbuffer()を使うと、カスタムパスが直接BackBufferへ出力でき、このFinalBlitPassを省略できる  
* ただしこのAPIはカメラのActiveColor/Depth TextureのIDを書き換える宣言にすぎない。最後のカメラの最終パスでのみ呼ぶことと、Y反転・sRGB変換・ViewportなどFinalBlitPassが担っていた処理を自前で持つことが前提になる

フルスクリーンのBlit1回は、高解像度・モバイル環境では無視できないコストです。カスタムポストプロセスを持つプロジェクトなら、検討する価値は十分にあります。

# **参考資料**

* [Unity Manual: Get data from the current frame in URP](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/accessing-frame-data.html)  
* [Class UniversalResourceData | Universal Render Pipeline | 17.3](https://docs.unity3d.com/Packages/com.unity.render-pipelines.universal@17.3/api/UnityEngine.Rendering.Universal.UniversalResourceData.html)  
* [Struct RasterGraphContext | Scriptable Render Pipeline Core | 17.3](https://docs.unity3d.com/Packages/com.unity.render-pipelines.core@17.3/api/UnityEngine.Rendering.RenderGraphModule.RasterGraphContext.html)

[image1]: assets/zhang-yubing-2/media/image1.png

[image2]: assets/zhang-yubing-2/media/image2.png