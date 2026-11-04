# **ParticleSystem 以較長時間執行 Simulate 時會產生巨大尖峰** 

羅馬拼音：KATO YUYA  
所屬：株式會社 Colorful Palette

# **前言**

Unity 的 ParticleSystem 提供了 ParticleSystem.Simulate，可將粒子的模擬時間控制在任意時間點。  
然而，若對 ParticleSystem.Simulate 指定較大的秒數，在某些情況下，開始模擬時會產生巨大的尖峰。  
本文將介紹其具體案例與注意事項。

# **什麼是 ParticleSystem.Simulate？**

ParticleSystem.Simulate 的官方文件{{fn:particleSystem_simulate_doc}}  
使用 ParticleSystem.Simulate 時，會將粒子效果模擬到第 1 個引數所指定的時間，並可從該指定時間重新播放粒子。  
CODE START csharp | 從 0 秒的時間點重新播放 {{list:particle_restart}}  
ParticleSystem.Simulate(0f, false, true);  
CODE END  
如 {{list:particle_restart}} 所示指定 0 秒，即可建立讓播放到一半的粒子從頭重新播放的機制。

CODE START csharp | 從 5 秒的時間點重新播放 {{list:particle_restart_five_s}}  
ParticleSystem.Simulate(5f, false, true);  
CODE END  
如 {{list:particle_restart_five_s}} 所示指定 5 秒，即可建立讓播放中的粒子從 5 秒的時間點重新播放的機制。

# **有問題的使用方式**

![][image1]  
図:有問題的粒子  
在 Unity 預設可建立的粒子上，額外開啟 Noise 模組。  
其他調整的參數，是為了讓粒子更容易觀察，在 Emission 模組中將發射調整為排成一直線。

{{pagebreak}}

CODE START csharp | 從 20 秒的時間點重新播放 {{list:particle_restart_twenty_s}}  
ParticleSystem.Simulate(20f, false, false);  
CODE END  
而若對這類使用了 Noise 模組的粒子，像 {{list:particle_restart_twenty_s}} 那樣在第 1 個引數指定過大的秒數進行模擬，處理過程中便會產生巨大的尖峰。

CODE START csharp | 模擬的負載測量{{list:particle_profiling}}  
void OnClickA()  
{  
    // 若有播放中的粒子則銷毀  
    if (\_activeParticle \!= null)  
    {  
        Destroy(\_activeParticle);  
        \_activeParticle \= null;  
    }

    // 新建立並播放  
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
        // 重新模擬  
        ps.Simulate(20f, false, false);  
        Profiler.EndSample();  
          
        ps.Play();  
    }  
}  
CODE END  
使用 Profiler 確認會產生多大的負載。  
這段程式碼以 ButtonA 建立前述粒子，以 ButtonB 模擬粒子。  
為了在 Unity 的 Profiler 中確認時更容易查明問題是否出在 ParticleSystem.Simulate，以 Profiler.BeginSample、Profiler.EndSample 將其包起來，方便調查。  
![][image2]  
図:尖峰發生的情形  
![][image3]図:處理時間  
如上所示，執行 ParticleSystem.Simulate 時，ParticleSystem 的 Noise 模組中 NoiseJob 被呼叫了約 2000 次，因此產生了約 50ms 的尖峰。  
藉由調低 ParticleSystem 的 Max Particles，可以稍微減少這個呼叫次數。

# **結語**

若需要使用 ParticleSystem.Simulate 建立從任意秒數開始產生粒子的機制，

* 是否有指定較長時間、從中途開始產生粒子的情境？  
* 是否使用了 Noise 模組？

請先確認上述幾點；特別是使用已啟用 Noise 模組的粒子時，最好避免使用 ParticleSystem.Simulate。

{{fn:particleSystem_simulate_doc}}  
https://docs.unity3d.com/ja/2022.3/ScriptReference/ParticleSystem.Simulate.html

[image1]: assets/kato-yuya-1/media/image1.png

[image2]: assets/kato-yuya-1/media/image2.png

[image3]: assets/kato-yuya-1/media/image3.png