# **对 ParticleSystem 执行长时间的 Simulate 会产生巨大尖峰** 

罗马字：KATO YUYA  
所属：株式会社 Colorful Palette

# **前言**

Unity 的 ParticleSystem 提供了 ParticleSystem.Simulate，可以将粒子的模拟时间控制到任意时刻。  
但是，如果给 ParticleSystem.Simulate 指定较大的秒数，在某些情况下开始模拟时会产生很大的尖峰。  
本文介绍其具体案例和注意事项。

# **什么是 ParticleSystem.Simulate？**

ParticleSystem.Simulate 的官方文档{{fn:particleSystem_simulate_doc}}  
使用 ParticleSystem.Simulate，可以将粒子效果模拟到第 1 个参数指定的时刻，并从该时刻起重新播放粒子。  
CODE START csharp | 从 0 秒时刻重新播放 {{list:particle_restart}}  
ParticleSystem.Simulate(0f, false, true);  
CODE END  
像{{list:particle_restart}}这样指定 0 秒，就可以实现将播放到一半的粒子从头重新播放的机制。

CODE START csharp | 从 5 秒时刻重新播放 {{list:particle_restart_five_s}}  
ParticleSystem.Simulate(5f, false, true);  
CODE END  
像{{list:particle_restart_five_s}}这样指定 5 秒，就可以实现将正在播放的粒子从 5 秒时刻重新播放的机制。

# **有问题的使用方式**

![][image1]  
図:有问题的粒子  
在 Unity 默认创建的粒子基础上，额外开启 Noise 模块。  
其他调整过的参数只有 Emission 模块：为了便于观察粒子，将其发射调整为排成一条直线。

{{pagebreak}}

CODE START csharp | 从 20 秒时刻重新播放 {{list:particle_restart_twenty_s}}  
ParticleSystem.Simulate(20f, false, false);  
CODE END  
对于这种使用了 Noise 模块的粒子，如果像{{list:particle_restart_twenty_s}}这样在第 1 个参数中指定过大的秒数进行模拟，处理过程中就会产生很大的尖峰。

CODE START csharp | 模拟开销的测量{{list:particle_profiling}}  
void OnClickA()  
{  
    // 如果有正在播放的粒子则销毁  
    if (\_activeParticle \!= null)  
    {  
        Destroy(\_activeParticle);  
        \_activeParticle \= null;  
    }

    // 新建并播放  
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
        // 重新模拟  
        ps.Simulate(20f, false, false);  
        Profiler.EndSample();  
          
        ps.Play();  
    }  
}  
CODE END  
用 Profiler 确认开销有多大。  
这段代码通过 ButtonA 生成所创建的粒子，通过 ButtonB 对粒子进行模拟。  
为了在 Unity 的 Profiler 中查看时更容易判断问题是否出在 ParticleSystem.Simulate，用 Profiler.BeginSample、Profiler.EndSample 将其包起来，便于排查。  
![][image2]  
図:尖峰产生时的情形  
![][image3]図:处理时间  
如上所示，调用 ParticleSystem.Simulate 时，ParticleSystem 的 Noise 模块中 NoiseJob 被调用了约 2000 次，由此产生了约 50ms 的尖峰。  
通过降低 ParticleSystem 的 Max Particles，可以稍微减少这个调用次数。

# **结语**

当需要使用 ParticleSystem.Simulate 来实现从任意秒数开始发射粒子的机制时，

* 是否存在指定较长时间、从中途开始发射粒子的场合？  
* 是否使用了 Noise 模块？

请确认以上两点；尤其是使用启用了 Noise 模块的粒子时，最好避免使用 ParticleSystem.Simulate。

{{fn:particleSystem_simulate_doc}}  
https://docs.unity3d.com/ja/2022.3/ScriptReference/ParticleSystem.Simulate.html

[image1]: assets/kato-yuya-1/media/image1.png

[image2]: assets/kato-yuya-1/media/image2.png

[image3]: assets/kato-yuya-1/media/image3.png