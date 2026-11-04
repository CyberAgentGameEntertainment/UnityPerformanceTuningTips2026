标题：使用 ResetControllerState 安全地实现 Animator 对象池  
罗马字：Yuki Shiotsuka  
所属：QualiArts

# 前言

本文介绍 Unity 6.3 新增的 Animator.ResetControllerState。

本文将 ResetControllerState 定位为“用于清晰、简单地实现归还对象池时初始化的 API”，并基于在 Unity 6000.3.21f1 中确认的行为，说明安全的对象池归还处理。

# Animator 对象池中出现的问题

ObjectPool 是通过复用已创建的实例来减少频繁创建与销毁开销的常用手法。  
Unity 的 ObjectPool 也提供了在获取、归还、销毁时机插入处理的机制{{fn:object_pool}}。

如果为了放入对象池而将 GameObject 设为非激活状态，再次激活时 Animator 会执行 Rebind，可能抵消对象池带来的性能收益。  
因此，Unity 官方的 Animator 类说明中建议禁用组件而不是 GameObject{{fn:animator_class}}。  
本文遵循这一建议，以如下对象池为前提：GameObject 保持激活状态存放，归还时禁用 Animator 和 Renderer，获取时再重新启用。

另一方面，既然对象池会复用实例，GameObject 和组件就可能带着上次使用时的状态回到池中。  
Animator 也是如此，禁用后停止的只有求值，内部状态会原样保留。  
如果不做任何处理，下次使用会沿用上次的 State 和各类 Parameter，可能从上次中途的 State 开始播放，或因残留的 Parameter 引发非预期的过渡。

ResetControllerState 是能将 AnimatorController 各 Layer 的 State 和 Parameter 一并恢复为默认状态的 API。  
发行说明中也将其介绍为便于对动画角色进行对象池管理而新增的功能{{fn:unity_6000_3_reset_controller_state_release_note}}。

# ResetControllerState 会重置的内容

将 ResetControllerState 的参数 resetParameters 指定为 true 时，除 State 外，Parameter 也会恢复为默认值。  
指定为 false 时，只有 State 会恢复{{fn:animator_reset_controller_state}}。  
将带有 Animator 的角色归还到对象池时，通常使用 ResetControllerState(true)，将 State 和 Parameter 一并恢复。  
无论用于未使用、普通过渡、AnyState 过渡还是 BlendTree，都会将 Float、Bool、Int 恢复为默认值，并重置 Trigger。  
此外，还会将所有 Layer 的当前 State 恢复为默认 State。

不过，各项内容生效的时机并不相同。  
Parameter 的值在调用后立即变为默认值，而 GetCurrentAnimatorStateInfo 在 Animator 下次求值之前，返回的仍是重置前的 State。

AnimatorControllerPlayable 也提供了用于重置 Controller 的 State 和 Parameter 的 ResetControllerState 方法{{fn:animator_controller_playable_reset_controller_state}}。

# ResetControllerState 不会重置的内容

另一方面，以下各项仅靠 ResetControllerState 不会恢复为默认值。

\- 通过 SetLayerWeight 修改的 Layer Weight  
\- Animator.speed  
\- 在 StateMachineBehaviour 中实现的自定义运行时变量

Layer Weight 和 Animator.speed 是独立于 Controller 的 State 和 Parameter 之外的属性。  
如果不希望归还对象池时保留修改后的值，需要显式恢复。  
StateMachineBehaviour 的自定义变量同样需要单独初始化。

此外，Rigidbody、ParticleSystem、AudioSource 等其他组件的状态以及自定义逻辑的状态，由于在 AnimatorController 之外管理，不在 ResetControllerState 的处理范围内。

# 实现示例

{{list:animator_pool_return_reset}}是将归还对象池时的初始化与禁用汇总在一起的示例。  
设想从 ObjectPool\<T\> 的 actionOnRelease 调用。  
其中也包含根据项目规范可能不需要的处理。

{{enlargepage}}

CODE START csharp | 归还对象池时的 Animator 初始化示例 {{list:animator_pool_return_reset}}  
// 汇总归还对象池时所需的初始化与禁用  
public void ReturnToPool()  
{  
    // 恢复 AnimatorController 各 Layer 的 State 和 Parameter  
    animator.ResetControllerState(true);  
    // defaultLayerWeight 作为复用时要恢复的值，需另行设置  
    animator.SetLayerWeight(layerIndex, defaultLayerWeight);  
    // 恢复播放速度  
    animator.speed \= 1f;  
    // 仅在需要时使用。在归还处理中完成重置后的首次求值  
    animator.Update(0f);  
    // 恢复 Animator 之外的自定义状态  
    ResetGameState();  
    // GameObject 保持激活状态，停止求值与绘制后存放  
    animator.enabled \= false;  
    characterRenderer.enabled \= false;  
}  
CODE END

# 总结

ObjectPool 并非只要减少创建与销毁的次数就算完成。  
需要确定复用的 GameObject 在下次使用时应从何种状态开始，并将恢复到该状态的处理一并纳入设计。

如果不将 GameObject 设为非激活状态，而是禁用 Animator 后存放，Animator 的内部状态会原样保留。  
使用 ResetControllerState(true) 可以将 AnimatorController 各 Layer 的 State，以及 Float、Bool、Int、Trigger 这些 Parameter 一并恢复为默认值。  
另一方面，Layer Weight、Animator.speed、StateMachineBehaviour 的自定义变量，以及其他组件和自定义逻辑的状态不会被重置。

将初始化范围划分为 AnimatorController 内部与外部，可以让归还对象池处理的职责更加明确。建议以 ResetControllerState 为核心，只补充项目特有的状态来构建归还处理。

# 参考资料

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
