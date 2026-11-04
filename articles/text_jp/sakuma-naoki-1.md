# **端末に応じた描画設定の切り替え**

* 佐久間尚輝 / NAOKI SAKUMA / 株式会社サイバーエージェント SGEコア技術本部

スマートフォン向けゲームでは幅広い端末でプレイされることが想定されます。  
一律で同じ描画設定にすると、低スペック端末ではクラッシュ、処理落ち、発熱の原因となってしまうケースがあります。  
逆に高スペック端末では端末の性能を活かしてより良い品質にすることができる可能性があります。  
そこで、端末に応じた適切な描画設定に切り替える仕組みを紹介します。

# **切り替えの手法**

低・中・高のようなプリセット形式と、ユーザーが直接値を操作できる形式の2つがゲームではよくある手法かと思います。

## **プリセット形式**

プリセット単位で切り替えたい場合の手法です。  
ユーザーにプリセットを選ばせる場合や、端末での自動判定でもこの手法を用います。

ProjectSettingsのQualityから、Add Quality Level を押下し、プリセット分追加します。  
その後、各プリセットに沿ったRenderPipelineAssetをアタッチします。  
必要に応じて、Qualityのその他の項目（Rendering, Textures等）を設定することも可能です。  
![][image1]  
コード上で、QualitySettings.SetQualityLevel を呼び出すことにより、任意のQualityLevelに切り替えることができます。

## **値を直接操作する形式**

ユーザーにパラメータを操作させる場合は、こちらの手法になります。  
以下のようなコードで、パラメータを直接操作することができます。

CODE START csharp  
var urpAsset \= (UniversalRenderPipelineAsset)GraphicsSettings.currentRenderPipeline;  
// 解像度の変更  
urpAsset.renderScale \= value;  
// 影のカスケードの枚数の変更  
urpAsset.shadowCascadeCount \= value;  
// その他、影の解像度であったり、追加ライトの個数等  
CODE END

# **端末に応じた切り替え**

端末によって、プリセットを推奨、自動で設定したいケースもあるかと思います。  
その場合は、 SystemInfo型が便利です。

特に、deviceModel、graphicsDeviceName、systemMemorySizeで見ると識別しやすい値を取得することができます。  
以下は返ってくる値の例です。  
表: 端末毎のSystemInfoの返ってくる値の例{{table:device_system_info}}

| 機種 | deviceModel | graphicsDeviceName | systemMemorySize |
| :---- | :---- | :---- | :---- |
| iPhone 13 | iPhone14,5 | Apple A15 GPU | 約3800 |
| iPad Pro 11-inch (第3世代) | iPad13,4 | Apple M1 GPU | 約7800 |
| Galaxy S23 | samsung SM-S911B | Adreno (TM) 740 | 約7500 |
| Pixel 8 | Google Pixel 8 | Mali-G715 | 約7400 |

iOSはdeviceModelとgraphicsDeviceNameの命名規則が一貫しているのと、基本的に同じ系列内であれば A もしくは M の後の数字が大きいほどスペックが高いです。  
ですので、組み合わせることで一定程度切り分けられます。

Androidは、各機種によって返ってくる値がまばらですので、iOSのように一概に比較することは難しいです。  
ですので、  
\* RAMで割り切る  
\* マスタに逃がして比較する  
\* 起動時にベンチマークツールを走らせる

等で切り分けるのが良いです。

# **自作RendererFeature**

自作のRendererFeature等を作成した際にも、描画設定を切り替えられるAPIを準備しておくと便利です。  
例えば、

*  一時テクスチャのフォーマットを下げる、解像度を下げる  
*  Blitの数を減らす  
*  簡易なシェーダーに置き換える

等の対応が挙げられます。

[image1]: assets/sakuma-naoki-1/media/image1.png