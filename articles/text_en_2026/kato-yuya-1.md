# **Simulating a Long Duration in ParticleSystem Causes a Huge Spike** 

Name: KATO YUYA  
Affiliation：Colorful Palette

# **Introduction**

Unity's ParticleSystem provides ParticleSystem.Simulate, which lets you control the particle simulation time to an arbitrary value.  
However, if you specify a large number of seconds for ParticleSystem.Simulate, a large spike can occur when the simulation starts.  
This article describes a concrete example of this and points to note.

# **What Is ParticleSystem.Simulate?**

Official documentation for ParticleSystem.Simulate {{fn:particleSystem_simulate_doc}}  
With ParticleSystem.Simulate, you can simulate the particle effect up to the time specified by the first argument and then play the particles again from that time.  
CODE START csharp | Replay from the 0-second point {{list:particle_restart}}  
ParticleSystem.Simulate(0f, false, true);  
CODE END  
Specifying 0 seconds, as in {{list:particle_restart}}, lets you build a mechanism that replays particles that were partway through playback from the beginning.

CODE START csharp | Replay from the 5-second point {{list:particle_restart_five_s}}  
ParticleSystem.Simulate(5f, false, true);  
CODE END  
Specifying 5 seconds, as in {{list:particle_restart_five_s}}, lets you build a mechanism that replays the playing particles from the 5-second point.

# **Problematic Usage**

![][image1]  
図:Problematic particles  
The Noise module is additionally turned on for a particle created with Unity's defaults.  
As for the other adjusted parameters, to make the particles easier to see, the emission is adjusted in the Emission module so that the particles are emitted in a straight line.

{{pagebreak}}

CODE START csharp | Replay from the 20-second point {{list:particle_restart_twenty_s}}  
ParticleSystem.Simulate(20f, false, false);  
CODE END  
If particles that use the Noise module are simulated with an excessively large number of seconds specified in the first argument, as in {{list:particle_restart_twenty_s}}, a large spike occurs during processing.

CODE START csharp | Measuring the simulation load {{list:particle_profiling}}  
void OnClickA()  
{  
    // Destroy the playing particle if there is one  
    if (\_activeParticle \!= null)  
    {  
        Destroy(\_activeParticle);  
        \_activeParticle \= null;  
    }

    // Create a new one and play it  
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
        // Re-simulate  
        ps.Simulate(20f, false, false);  
        Profiler.EndSample();  
          
        ps.Play();  
    }  
}  
CODE END  
We check how much load is applied by using the Profiler.  
This is code in which ButtonA creates a particle and ButtonB simulates it.  
To make it easier to check in Unity's Profiler whether ParticleSystem.Simulate is the problem, we wrap it with Profiler.BeginSample and Profiler.EndSample to make the investigation easier.  
![][image2]  
図:The spike occurring  
![][image3] Figure: Processing time  
As shown, during ParticleSystem.Simulate, NoiseJob is called about 2000 times by the Noise module of the ParticleSystem, which causes a spike of about 50 ms.  
This number of calls can be reduced somewhat by lowering the ParticleSystem's Max Particles.

# **Conclusion**

If you need to build a mechanism that emits particles from an arbitrary number of seconds by using ParticleSystem.Simulate,

* Is there a case where a long duration is specified and particles are emitted from midway?  
* Is the Noise module used?

Check these two points, and avoid using ParticleSystem.Simulate, especially when using particles with the Noise module enabled.

{{fn:particleSystem_simulate_doc}}  
https://docs.unity3d.com/ja/2022.3/ScriptReference/ParticleSystem.Simulate.html

[image1]: assets/kato-yuya-1/media/image1.png

[image2]: assets/kato-yuya-1/media/image2.png

[image3]: assets/kato-yuya-1/media/image3.png