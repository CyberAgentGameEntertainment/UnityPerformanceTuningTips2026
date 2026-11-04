タイトル：ResetControllerStateでAnimatorプーリングを安全に行う  
名前：塩塚 勇気  
ローマ字：Yuki Shiotsuka  
所属：QualiArts

# はじめに

本記事では、Unity 6.3で追加されたAnimator.ResetControllerStateを取り上げます。

ResetControllerStateを「プール返却時の初期化を明確かつ簡単に実装するためのAPI」として整理し、Unity 6000.3.21f1で確認した挙動をもとに、安全なプール返却処理を説明します。

# Animatorプーリングで起きる問題

ObjectPoolは、生成済みのインスタンスを使い回すことで、頻繁な生成と破棄の負荷を減らすための定番手法です。  
UnityのObjectPoolも、取得、返却、破棄のタイミングに処理を差し込める仕組みを持ちます{{fn:object_pool}}。

プールのためにGameObjectを非アクティブにすると、再びアクティブにしたときにAnimatorがRebindを行い、プールで得た性能を打ち消す可能性があります。  
そのため、Unity公式のAnimatorクラスの説明では、GameObjectではなくコンポーネントを無効にすることが推奨されています{{fn:animator_class}}。  
本記事では、この推奨に沿ってGameObjectはアクティブのまま保管し、返却時にAnimatorとRendererを無効にして、取得時に再び有効にするプールを前提とします。

一方で、プールでインスタンスを再利用する以上、GameObjectやコンポーネントは前回利用時の状態を持ったまま戻ってくる可能性があります。  
Animatorの場合も、無効にして止まるのは評価だけで、内部の状態はそのまま残ります。  
何もしなければ、前回利用時のStateや各種Parameterを引き継いだまま次の利用が始まるため、前回の途中のStateから再生されたり、残ったParameterによって意図しない遷移が起きることがあります。

ResetControllerStateは、AnimatorControllerの各LayerのStateとParameterをまとめて既定の状態へ戻せるAPIです。  
リリースノートでも、アニメーションするキャラクターのプーリングを容易にするための追加として紹介されています{{fn:unity_6000_3_reset_controller_state_release_note}}。

# ResetControllerStateでリセットされる項目

ResetControllerStateの引数resetParametersにtrueを指定すると、Stateに加えてParameterも既定値へ戻ります。  
falseを指定した場合は、Stateだけが戻ります{{fn:animator_reset_controller_state}}。  
Animator付きキャラクターをプールへ返す場合は、通常はResetControllerState(true)を使い、StateとParameterをまとめて戻します。  
未使用、通常遷移、AnyState遷移、BlendTreeの用途にかかわらず、Float、Bool、Intを既定値へ戻し、Triggerをリセットします。  
また、すべてのLayerの現在Stateを既定のStateへ戻します。

ただし、反映されるタイミングは項目によって異なります。  
Parameterの値は呼び出し直後から既定値になりましたが、GetCurrentAnimatorStateInfoは、次にAnimatorが評価されるまでリセット前のStateを返しました。

AnimatorControllerPlayableにも、ControllerのStateとParameterをリセットするResetControllerStateメソッドが用意されています{{fn:animator_controller_playable_reset_controller_state}}。

# ResetControllerStateではリセットされない項目

一方、次の項目はResetControllerStateだけでは既定値へ戻りません。

\- SetLayerWeightで変更したLayer Weight  
\- Animator.speed  
\- StateMachineBehaviourに実装した独自の実行時変数

Layer WeightやAnimator.speedは、ControllerのStateやParameterとは別のプロパティです。  
プール返却時に変更後の値を残したくない場合は、明示的に戻します。  
StateMachineBehaviourの独自変数も、個別の初期化が必要です。

また、Rigidbody、ParticleSystem、AudioSourceなど他のコンポーネントの状態や、独自ロジックの状態は、AnimatorControllerの外で管理されるため、ResetControllerStateの対象ではありません。

# 実装例

{{list:animator_pool_return_reset}}は、プール返却時の初期化と無効化をまとめた例です。  
ObjectPool\<T\>のactionOnReleaseから呼び出すことを想定しています。  
プロジェクトの仕様によっては不要な処理も含まれます。

{{enlargepage}}

CODE START csharp | プール返却時のAnimator初期化例 {{list:animator_pool_return_reset}}  
// プール返却時に必要な初期化と無効化をまとめる  
public void ReturnToPool()  
{  
    // AnimatorControllerの各LayerのStateとParameterを戻す  
    animator.ResetControllerState(true);  
    // defaultLayerWeightは、再利用時に戻す値として別途設定しておく  
    animator.SetLayerWeight(layerIndex, defaultLayerWeight);  
    // 再生速度を戻す  
    animator.speed \= 1f;  
    // 必要な場合のみ。リセット後の最初の評価を、返却処理の中で済ませる  
    animator.Update(0f);  
    // Animator外の独自状態を戻す  
    ResetGameState();  
    // GameObjectはアクティブのまま、評価と描画を止めて保管する  
    animator.enabled \= false;  
    characterRenderer.enabled \= false;  
}  
CODE END

# まとめ

ObjectPoolは、生成と破棄の回数を減らすだけでは完成しません。  
再利用するGameObjectが次回利用時にどの状態から始まるべきかを決め、その状態へ戻す処理まで含めて設計する必要があります。

GameObjectを非アクティブにせず、Animatorを無効にして保管する場合、Animatorの内部状態はそのまま残ります。  
ResetControllerState(true)を使うと、AnimatorControllerの各LayerのStateと、Float、Bool、Int、TriggerのParameterをまとめて既定値へ戻せます。  
一方、Layer Weight、Animator.speed、StateMachineBehaviourの独自変数、他のコンポーネントや独自ロジックの状態はリセットされません。

初期化対象をAnimatorController内と外に分けておくと、プール返却処理の責務が明確になります。ResetControllerStateを軸に、プロジェクト固有の状態だけを補う形で返却処理を組み立てるとよいでしょう。

# 参考資料

* [Unity Scripting API \- ObjectPool\<T0\>](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Pool.ObjectPool_1.html)  
* [Unity Scripting API \- Animator](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Animator.html)  
* [Unity 6000.3.0f1 Release Notes](https://unity.com/releases/editor/whats-new/6000.3.0f1)  
* [Unity Scripting API \- Animator.ResetControllerState](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Animator.ResetControllerState.html)  
* [Unity Scripting API \- AnimatorControllerPlayable.ResetControllerState](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Animations.AnimatorControllerPlayable.ResetControllerState.html)

脚注: {{fn:object_pool}} https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Pool.ObjectPool\_1.html

脚注: {{fn:animator_class}} https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Animator.html

脚注: {{fn:unity_6000_3_reset_controller_state_release_note}} https://unity.com/releases/editor/whats-new/6000.3.0f1

脚注: {{fn:animator_reset_controller_state}} https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Animator.ResetControllerState.html

脚注: {{fn:animator_controller_playable_reset_controller_state}} https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Animations.AnimatorControllerPlayable.ResetControllerState.html
