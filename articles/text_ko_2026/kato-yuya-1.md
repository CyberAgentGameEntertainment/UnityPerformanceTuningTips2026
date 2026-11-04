# **ParticleSystem에서 긴 시간을 Simulate하면 거대한 스파이크가 발생한다** 

로마자: KATO YUYA  
소속：주식회사 Colorful Palette

# **들어가며**

Unity의 ParticleSystem에는 파티클의 시뮬레이션 시간을 임의의 시간으로 제어할 수 있는 ParticleSystem.Simulate가 있습니다.  
그러나 ParticleSystem.Simulate에 큰 초 값을 지정하면 시뮬레이션을 시작할 때 큰 스파이크가 발생하는 경우가 있습니다.  
이 글에서는 그 구체적인 사례와 주의점을 소개합니다.

# **ParticleSystem.Simulate란?**

ParticleSystem.Simulate에 대한 공식 문서{{fn:particleSystem_simulate_doc}}  
ParticleSystem.Simulate를 사용하면 첫 번째 인자로 지정한 시간까지 파티클 연출을 시뮬레이션하고, 지정한 시간부터 파티클을 다시 재생할 수 있습니다.  
CODE START csharp | 0초 시점부터 다시 재생 {{list:particle_restart}}  
ParticleSystem.Simulate(0f, false, true);  
CODE END  
{{list:particle_restart}}처럼 0초를 지정하면 도중까지 재생하던 파티클을 처음부터 다시 재생하는 구조를 만들 수 있습니다.

CODE START csharp | 5초 시점부터 다시 재생 {{list:particle_restart_five_s}}  
ParticleSystem.Simulate(5f, false, true);  
CODE END  
{{list:particle_restart_five_s}}처럼 5초를 지정하면 재생 중인 파티클을 5초 시점부터 다시 재생하는 구조를 만들 수 있습니다.

# **문제가 있는 사용 방법**

![][image1]  
図:문제가 있는 파티클  
Unity에서 기본으로 생성되는 파티클에 Noise 모듈을 추가로 켭니다.  
그 외에 조정한 파라미터로, 파티클을 보기 쉽게 하기 위해 Emission 모듈에서 파티클이 일직선으로 발생하도록 조정했습니다.

{{pagebreak}}

CODE START csharp | 20초 시점부터 다시 재생 {{list:particle_restart_twenty_s}}  
ParticleSystem.Simulate(20f, false, false);  
CODE END  
이러한 Noise 모듈을 사용한 파티클을 {{list:particle_restart_twenty_s}}처럼 첫 번째 인자의 초 값에 지나치게 큰 값을 넣어 시뮬레이션하면 처리 중에 큰 스파이크가 발생합니다.

CODE START csharp | 시뮬레이션 부하 계측{{list:particle_profiling}}  
void OnClickA()  
{  
    // 재생 중인 파티클이 있으면 파괴  
    if (\_activeParticle \!= null)  
    {  
        Destroy(\_activeParticle);  
        \_activeParticle \= null;  
    }

    // 새로 생성하여 재생  
    \_activeParticle \= Instantiate(particlePrefab, Vector3.zero, Quaternion.identity);  
    var ps \= \_activeParticle.GetComponent\<ParticleSystem\>();  
    if (ps \!= null)  
    {  
        ps.Play();  
    }  
}

void OnClickB()  
{  
    if (\_activeParticle \== null) return;

    var ps \= \_activeParticle.GetComponent\<ParticleSystem\>();  
    if (ps \!= null)  
    {  
        Profiler.BeginSample("再シミュレーション");  
        // 재시뮬레이션  
        ps.Simulate(20f, false, false);  
        Profiler.EndSample();  
          
        ps.Play();  
    }  
}  
CODE END  
부하가 어느 정도인지 프로파일러로 확인합니다.  
ButtonA로 파티클을 생성하고 ButtonB로 파티클을 시뮬레이션하는 코드입니다.  
Unity 프로파일러로 확인할 때 ParticleSystem.Simulate에 문제가 있는지 쉽게 조사할 수 있도록 Profiler.BeginSample과 Profiler.EndSample로 감쌉니다.  
![][image2]  
図:스파이크가 발생하는 모습  
![][image3]그림: 처리 시간  
이처럼 ParticleSystem.Simulate 시 ParticleSystem의 Noise 모듈에서 NoiseJob이 약 2000회 호출되며, 이로 인해 약 50ms의 스파이크가 발생합니다.  
이 호출 횟수는 ParticleSystem의 Max Particles를 낮추면 어느 정도 줄일 수 있습니다.

# **마치며**

ParticleSystem.Simulate를 사용하여 임의의 초 시점부터 파티클을 발생시키는 구조를 만들어야 하는 경우,

* 긴 시간을 지정하여 도중부터 파티클을 발생시키는 상황이 있는가?  
* Noise 모듈을 사용하고 있는가?

를 확인하고, 특히 Noise 모듈을 활성화한 파티클을 사용하는 경우에는 ParticleSystem.Simulate의 사용을 삼가는 편이 좋습니다.

{{fn:particleSystem_simulate_doc}}  
https://docs.unity3d.com/ja/2022.3/ScriptReference/ParticleSystem.Simulate.html

[image1]: assets/kato-yuya-1/media/image1.png

[image2]: assets/kato-yuya-1/media/image2.png

[image3]: assets/kato-yuya-1/media/image3.png