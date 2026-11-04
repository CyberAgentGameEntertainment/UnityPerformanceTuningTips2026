標題：使用 ResetControllerState 安全地進行 Animator 物件池化  
羅馬拼音：Yuki Shiotsuka  
所屬：QualiArts

# 前言

本文將介紹 Unity 6.3 新增的 Animator.ResetControllerState。

本文將 ResetControllerState 定位為「用來明確且簡單地實作歸還至物件池時的初始化的 API」，並根據在 Unity 6000.3.21f1 上確認的行為，說明安全的物件池歸還處理。

# Animator 物件池化時發生的問題

ObjectPool 是透過重複使用已建立的實例，來減少頻繁建立與銷毀負載的常見手法。  
Unity 的 ObjectPool 也具備可在取得、歸還、銷毀的時機插入處理的機制{{fn:object_pool}}。

若為了放入物件池而將 GameObject 設為非啟用，再次啟用時 Animator 會執行 Rebind，可能抵銷物件池帶來的效能。  
因此，Unity 官方的 Animator 類別說明中建議停用元件，而非停用 GameObject{{fn:animator_class}}。  
本文依循此建議，以下列物件池為前提：GameObject 保持啟用狀態存放，歸還時停用 Animator 與 Renderer，取得時再重新啟用。

另一方面，既然物件池會重複使用實例，GameObject 與元件就可能帶著上次使用時的狀態回來。  
Animator 也一樣，停用後停止的只有評估，內部狀態仍會保留。  
若什麼都不做，下次使用時會沿用上次使用時的 State 與各種 Parameter，因此可能從上次中途的 State 開始播放，或因殘留的 Parameter 而發生非預期的轉換。

ResetControllerState 是可將 AnimatorController 各 Layer 的 State 與 Parameter 一併還原為預設狀態的 API。  
在發行說明中，也將其介紹為讓動畫角色的物件池化更容易的新增功能{{fn:unity_6000_3_reset_controller_state_release_note}}。

# ResetControllerState 會重設的項目

若將 ResetControllerState 的引數 resetParameters 指定為 true，除了 State 之外，Parameter 也會還原為預設值。  
指定為 false 時，只會還原 State{{fn:animator_reset_controller_state}}。  
將附有 Animator 的角色歸還至物件池時，通常會使用 ResetControllerState(true)，將 State 與 Parameter 一併還原。  
無論用途是未使用、一般轉換、AnyState 轉換或 BlendTree，都會將 Float、Bool、Int 還原為預設值，並重設 Trigger。  
此外，也會將所有 Layer 的目前 State 還原為預設 State。

不過，反映的時機會因項目而異。  
Parameter 的值在呼叫後立即變為預設值，但 GetCurrentAnimatorStateInfo 在 Animator 下次被評估之前，仍會傳回重設前的 State。

AnimatorControllerPlayable 也提供了可重設 Controller 的 State 與 Parameter 的 ResetControllerState 方法{{fn:animator_controller_playable_reset_controller_state}}。

# ResetControllerState 不會重設的項目

另一方面，下列項目僅靠 ResetControllerState 不會還原為預設值。

\- 以 SetLayerWeight 變更的 Layer Weight  
\- Animator.speed  
\- 在 StateMachineBehaviour 中實作的自訂執行階段變數

Layer Weight 與 Animator.speed 是與 Controller 的 State、Parameter 不同的屬性。  
若不希望在歸還至物件池時保留變更後的值，請明確地將其還原。  
StateMachineBehaviour 的自訂變數也需要個別初始化。

此外，Rigidbody、ParticleSystem、AudioSource 等其他元件的狀態，以及自訂邏輯的狀態，是在 AnimatorController 之外管理，因此不在 ResetControllerState 的處理範圍內。

# 實作範例

{{list:animator_pool_return_reset}} 是將歸還至物件池時的初始化與停用整理在一起的範例。  
預期從 ObjectPool\<T\> 的 actionOnRelease 呼叫。  
依專案的規範不同，其中也包含不需要的處理。

{{enlargepage}}

CODE START csharp | 歸還至物件池時的 Animator 初始化範例 {{list:animator_pool_return_reset}}  
// 整理歸還至物件池時所需的初始化與停用  
public void ReturnToPool()  
{  
    // 還原 AnimatorController 各 Layer 的 State 與 Parameter  
    animator.ResetControllerState(true);  
    // defaultLayerWeight 需另外設定為重複使用時要還原的值  
    animator.SetLayerWeight(layerIndex, defaultLayerWeight);  
    // 還原播放速度  
    animator.speed \= 1f;  
    // 僅在需要時使用。在歸還處理中完成重設後的第一次評估  
    animator.Update(0f);  
    // 還原 Animator 之外的自訂狀態  
    ResetGameState();  
    // GameObject 保持啟用，停止評估與繪製後存放  
    animator.enabled \= false;  
    characterRenderer.enabled \= false;  
}  
CODE END

# 總結

ObjectPool 並非只靠減少建立與銷毀的次數就算完成。  
必須決定重複使用的 GameObject 在下次使用時應從何種狀態開始，並將還原至該狀態的處理一併納入設計。

若不將 GameObject 設為非啟用，而是停用 Animator 後存放，Animator 的內部狀態會原樣保留。  
使用 ResetControllerState(true)，可將 AnimatorController 各 Layer 的 State，以及 Float、Bool、Int、Trigger 的 Parameter 一併還原為預設值。  
另一方面，Layer Weight、Animator.speed、StateMachineBehaviour 的自訂變數，以及其他元件或自訂邏輯的狀態不會被重設。

將初始化的範圍區分為 AnimatorController 內與外，物件池歸還處理的職責就會更明確。建議以 ResetControllerState 為核心，只補上專案特有的狀態來組成歸還處理。

# 參考資料

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
