제목：ResetControllerState로 Animator 풀링을 안전하게 수행하기  
로마자：Yuki Shiotsuka  
소속：QualiArts

# 들어가며

이 글에서는 Unity 6.3에서 추가된 Animator.ResetControllerState를 다룹니다.

ResetControllerState를 '풀 반환 시 초기화를 명확하고 간단하게 구현하기 위한 API'로 정리하고, Unity 6000.3.21f1에서 확인한 동작을 바탕으로 안전한 풀 반환 처리를 설명합니다.

# Animator 풀링에서 발생하는 문제

ObjectPool은 생성해 둔 인스턴스를 재사용하여 잦은 생성과 파괴의 부하를 줄이는 대표적인 기법입니다.  
Unity의 ObjectPool도 가져오기, 반환, 파괴 시점에 처리를 끼워 넣을 수 있는 구조를 갖추고 있습니다{{fn:object_pool}}.

풀링을 위해 GameObject를 비활성화하면, 다시 활성화할 때 Animator가 Rebind를 수행하여 풀링으로 얻은 성능을 상쇄할 수 있습니다.  
이 때문에 Unity 공식 Animator 클래스 설명에서는 GameObject가 아니라 컴포넌트를 비활성화하도록 권장합니다{{fn:animator_class}}.  
이 글에서는 이 권장 사항에 따라 GameObject를 활성 상태로 보관하고, 반환 시 Animator와 Renderer를 비활성화하며, 획득 시 다시 활성화하는 풀을 전제로 합니다.

한편 풀에서 인스턴스를 재사용하는 이상, GameObject나 컴포넌트는 이전 사용 시의 상태를 유지한 채 돌아올 수 있습니다.  
Animator도 비활성화하면 멈추는 것은 평가뿐이며, 내부 상태는 그대로 남습니다.  
아무것도 하지 않으면 이전 사용 시의 State와 각종 Parameter를 그대로 이어받은 채 다음 사용이 시작되므로, 이전의 중간 State부터 재생되거나 남아 있는 Parameter 때문에 의도하지 않은 전이가 발생할 수 있습니다.

ResetControllerState는 AnimatorController의 각 Layer의 State와 Parameter를 한 번에 기본 상태로 되돌릴 수 있는 API입니다.  
릴리스 노트에서도 애니메이션하는 캐릭터의 풀링을 쉽게 하기 위한 추가 사항으로 소개되어 있습니다{{fn:unity_6000_3_reset_controller_state_release_note}}.

# ResetControllerState로 리셋되는 항목

ResetControllerState의 인자 resetParameters에 true를 지정하면 State에 더해 Parameter도 기본값으로 돌아갑니다.  
false를 지정한 경우에는 State만 돌아갑니다{{fn:animator_reset_controller_state}}.  
Animator가 있는 캐릭터를 풀에 반환할 때는 보통 ResetControllerState(true)를 사용하여 State와 Parameter를 함께 되돌립니다.  
미사용, 일반 전환, AnyState 전환, BlendTree 등 용도와 관계없이 Float, Bool, Int를 기본값으로 되돌리고 Trigger를 리셋합니다.  
또한 모든 Layer의 현재 State를 기본 State로 되돌립니다.

다만 반영되는 시점은 항목에 따라 다릅니다.  
Parameter 값은 호출 직후부터 기본값이 되었지만, GetCurrentAnimatorStateInfo는 다음에 Animator가 평가될 때까지 리셋 전의 State를 반환했습니다.

AnimatorControllerPlayable에도 Controller의 State와 Parameter를 리셋하는 ResetControllerState 메서드가 마련되어 있습니다{{fn:animator_controller_playable_reset_controller_state}}.

# ResetControllerState로 리셋되지 않는 항목

반면 다음 항목은 ResetControllerState만으로는 기본값으로 돌아가지 않습니다.

\- SetLayerWeight로 변경한 Layer Weight  
\- Animator.speed  
\- StateMachineBehaviour에 구현한 독자적인 런타임 변수

Layer Weight와 Animator.speed는 Controller의 State나 Parameter와는 별개의 프로퍼티입니다.  
풀 반환 시 변경된 값을 남기고 싶지 않다면 명시적으로 되돌립니다.  
StateMachineBehaviour의 자체 변수도 개별적인 초기화가 필요합니다.

또한 Rigidbody, ParticleSystem, AudioSource 등 다른 컴포넌트의 상태나 자체 로직의 상태는 AnimatorController 밖에서 관리되므로 ResetControllerState의 대상이 아닙니다.

# 구현 예

{{list:animator_pool_return_reset}}은 풀 반환 시의 초기화와 비활성화를 합친 예입니다.  
ObjectPool\<T\>의 actionOnRelease에서 호출하는 것을 전제로 합니다.  
프로젝트 사양에 따라서는 불필요한 처리도 포함되어 있습니다.

{{enlargepage}}

CODE START csharp | 풀 반환 시 Animator 초기화 예 {{list:animator_pool_return_reset}}  
// 풀 반환 시 필요한 초기화와 비활성화를 한데 모은다  
public void ReturnToPool()  
{  
    // AnimatorController의 각 Layer의 State와 Parameter를 되돌린다  
    animator.ResetControllerState(true);  
    // defaultLayerWeight는 재사용 시 되돌릴 값으로 별도로 설정해 둔다  
    animator.SetLayerWeight(layerIndex, defaultLayerWeight);  
    // 재생 속도를 되돌린다  
    animator.speed \= 1f;  
    // 필요한 경우에만. 리셋 후 첫 평가를 반환 처리 안에서 끝낸다  
    animator.Update(0f);  
    // Animator 외부의 자체 상태를 되돌린다  
    ResetGameState();  
    // GameObject는 활성 상태로 두고, 평가와 렌더링을 멈춘 채 보관한다  
    animator.enabled \= false;  
    characterRenderer.enabled \= false;  
}  
CODE END

# 정리

ObjectPool은 생성과 파괴 횟수를 줄이는 것만으로는 완성되지 않습니다.  
재사용하는 GameObject가 다음 사용 시 어떤 상태에서 시작해야 하는지를 정하고, 그 상태로 되돌리는 처리까지 포함하여 설계해야 합니다.

GameObject를 비활성화하지 않고 Animator를 비활성화하여 보관하는 경우, Animator의 내부 상태는 그대로 남습니다.  
ResetControllerState(true)를 사용하면 AnimatorController의 각 Layer의 State와 Float, Bool, Int, Trigger Parameter를 한 번에 기본값으로 되돌릴 수 있습니다.  
반면 Layer Weight, Animator.speed, StateMachineBehaviour의 자체 변수, 다른 컴포넌트나 자체 로직의 상태는 리셋되지 않습니다.

초기화 대상을 AnimatorController 내부와 외부로 나누어 두면 풀 반환 처리의 책임이 명확해집니다. ResetControllerState를 중심에 두고 프로젝트 고유의 상태만 보완하는 형태로 반환 처리를 구성하는 것이 좋습니다.

# 참고 자료

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
