Title: Safely Pooling Animators with ResetControllerState  
Name: Yuki Shiotsuka  
Affiliation：QualiArts

# Introduction

This article covers Animator.ResetControllerState, added in Unity 6.3.

This article organizes ResetControllerState as an API for implementing initialization on return to a pool clearly and easily, and explains safe pool-return handling based on the behavior confirmed in Unity 6000.3.21f1.

# Problems that occur with Animator pooling

ObjectPool is a standard technique that reduces the load of frequent creation and destruction by reusing instances that have already been created.  
Unity's ObjectPool also provides hooks that let you run code when an instance is retrieved, returned, or destroyed {{fn:object_pool}}.

If you deactivate a GameObject for pooling, the Animator performs a Rebind when the GameObject is activated again, which can cancel out the performance gained from pooling.  
For this reason, the official Unity description of the Animator class recommends disabling the component rather than the GameObject {{fn:animator_class}}.  
This article assumes a pool that follows this recommendation: GameObjects are kept active, the Animator and Renderer are disabled on return, and they are enabled again on acquisition.

On the other hand, because a pool reuses instances, a GameObject or component can come back still holding the state from its previous use.  
The same applies to the Animator: disabling it stops only the evaluation, and the internal state remains as it is.  
If nothing is done, the next use starts with the State and the various Parameters carried over from the previous use, so playback can start from a State left midway in the previous use, or an unintended transition can occur because of the remaining Parameters.

ResetControllerState is an API that returns the State and Parameters of each Layer of an AnimatorController to their default state in one call.  
The release notes also introduce it as an addition that makes it easier to pool animated characters {{fn:unity_6000_3_reset_controller_state_release_note}}.

# What ResetControllerState Resets

If you specify true for the resetParameters argument of ResetControllerState, the Parameters are returned to their default values in addition to the State.  
If you specify false, only the State is returned {{fn:animator_reset_controller_state}}.  
When returning a character with an Animator to the pool, normally use ResetControllerState(true) to return the State and Parameters together.  
Regardless of whether they are unused, used in regular transitions, AnyState transitions, or BlendTrees, Float, Bool, and Int parameters are returned to their default values and Trigger parameters are reset.  
It also returns the current State of all Layers to the default State.

However, the timing at which the reset takes effect differs by item.  
Parameter values became the default values immediately after the call, but GetCurrentAnimatorStateInfo returned the State from before the reset until the Animator was next evaluated.

AnimatorControllerPlayable also provides a ResetControllerState method that resets the State and Parameters of the Controller {{fn:animator_controller_playable_reset_controller_state}}.

# What ResetControllerState Does Not Reset

On the other hand, the following items are not returned to their default values by ResetControllerState alone.

\- Layer Weight changed with SetLayerWeight  
\- Animator.speed  
\- Custom runtime variables implemented in a StateMachineBehaviour

Layer Weight and Animator.speed are properties separate from the State and Parameters of the Controller.  
If you do not want changed values to remain when returning to the pool, restore them explicitly.  
Custom variables of a StateMachineBehaviour also require individual initialization.

In addition, the state of other components such as Rigidbody, ParticleSystem, and AudioSource, and the state of custom logic, are managed outside the AnimatorController and are therefore not covered by ResetControllerState.

# Implementation Example

{{list:animator_pool_return_reset}} is an example that combines initialization and disabling on return to the pool.  
It is intended to be called from actionOnRelease of ObjectPool\<T\>.  
Depending on the project's specifications, it includes processing that is not needed.

{{enlargepage}}

CODE START csharp | Animator initialization example on return to the pool {{list:animator_pool_return_reset}}  
// Combine the initialization and disabling required on return to the pool  
public void ReturnToPool()  
{  
    // Return the State and Parameters of each Layer of the AnimatorController  
    animator.ResetControllerState(true);  
    // Set defaultLayerWeight separately as the value to restore on reuse  
    animator.SetLayerWeight(layerIndex, defaultLayerWeight);  
    // Reset the playback speed  
    animator.speed \= 1f;  
    // Only when needed: complete the first evaluation after the reset within the return processing  
    animator.Update(0f);  
    // Restore custom state outside the Animator  
    ResetGameState();  
    // Keep the GameObject active, stop evaluation and rendering, and store it  
    animator.enabled \= false;  
    characterRenderer.enabled \= false;  
}  
CODE END

# Summary

An ObjectPool is not complete just by reducing the number of creations and destructions.  
You need to decide what state a reused GameObject should start from on its next use, and design the processing that returns it to that state.

If you store the Animator disabled without deactivating the GameObject, the internal state of the Animator remains as it is.  
Using ResetControllerState(true) returns the State of each Layer of the AnimatorController and the Float, Bool, Int, and Trigger Parameters to their default values together.  
On the other hand, Layer Weight, Animator.speed, custom variables of a StateMachineBehaviour, and the state of other components and custom logic are not reset.

Separating what to initialize into items inside and outside the AnimatorController clarifies the responsibilities of the pool-return handling. It is advisable to build the return handling around ResetControllerState and supplement it only with project-specific state.

# References

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
