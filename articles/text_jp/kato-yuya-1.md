# **ParticleSystemで長い時間Simulateをすると巨大なスパイクが発生する** 

名前：加藤 有也  
ローマ字：KATO YUYA  
所属：株式会社Colorful Palette

# **はじめに**

UnityのParticleSystemにはパーティクルのシミュレーション時間を任意の時間で制御することのできるParticleSystem.Simulateが用意されています。  
しかし、ParticleSystem.Simulateに大きな秒数を指定するとシミュレーションを開始した際に大きなスパイクが発生してしまうケースがあります。  
本記事では、その具体的な事例と注意点について紹介します。

# **ParticleSystem.Simulateとは？**

ParticleSystem.Simulateについての公式ドキュメント{{fn:particleSystem_simulate_doc}}  
ParticleSystem.Simulateを使用すると第1引数で指定した時間までパーティクルの演出をシミュレーションし、指定した時間からパーティクルを再度再生することが可能です。  
CODE START csharp | 0秒時点から再生し直す {{list:particle_restart}}  
ParticleSystem.Simulate(0f, false, true);  
CODE END  
{{list:particle_restart}}のように0秒を指定すると途中まで再生していたパーティクルを最初から再生し直す仕組みを作れます。

CODE START csharp | 5秒時点から再生し直す {{list:particle_restart_five_s}}  
ParticleSystem.Simulate(5f, false, true);  
CODE END  
{{list:particle_restart_five_s}}のように5秒を指定すると再生中のパーティクルを5秒時点から再生し直す仕組みを作れます。

# **問題がある使用方法**

![][image1]  
図:問題のあるパーティクル  
Unityのデフォルトで作成できるパーティクルに追加でNoiseモジュールをオンにします。  
他に調整しているパラメータはパーティクルを見やすくするためにEmissionモジュールで一直線になるように発生を調整しています。

{{pagebreak}}

CODE START csharp | 20秒時点から再生し直す {{list:particle_restart_twenty_s}}  
ParticleSystem.Simulate(20f, false, false);  
CODE END  
そしてこのようなNoiseモジュールを使用したパーティクルを{{list:particle_restart_twenty_s}}のように第1引数の秒数指定に大きすぎる秒数を入れてシミュレーションしてしまうと処理中に大きなスパイクが発生します。

CODE START csharp | シミュレーションの負荷計測{{list:particle_profiling}}  
void OnClickA()  
{  
    // 再生中のパーティクルがあれば破棄  
    if (\_activeParticle \!= null)  
    {  
        Destroy(\_activeParticle);  
        \_activeParticle \= null;  
    }

    // 新規生成して再生  
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
        // 再シミュレーション  
        ps.Simulate(20f, false, false);  
        Profiler.EndSample();  
          
        ps.Play();  
    }  
}  
CODE END  
どれほどの負荷がかかるのか、Profilerで確認します。  
ButtonAで作成したパーティクルを生成、ButtonBでパーティクルをシミュレートするコードです。  
UnityのProfilerで確認した時にParticleSystem.Simulateに問題があるのか調べやすくするためにProfiler.BeginSample、Profiler.EndSampleで囲い、調査しやすくします。  
![][image2]  
図:スパイクが発生している様子  
![][image3]図:処理時間  
このようにParticleSystem.Simulate時、ParticleSystemのNoiseモジュールでNoiseJobが約2000回コールされ、これにより約50msのスパイクが発生しています。  
このコール数はParticleSystemのMax Particlesを下げることで少しは減らすことが可能です。

# **おわりに**

ParticleSystem.Simulateを使用して任意の秒数からパーティクルを発生させる仕組みを作る必要がある場合、

* 長い時間を指定して途中からパーティクルを発生させる場面があるのか？  
* Noiseモジュールを使用しているのか？

を確認し、特にNoiseモジュールを有効にしているパーティクルを使用する場合はParticleSystem.Simulateの利用を控えた方が良いです。

{{fn:particleSystem_simulate_doc}}  
https://docs.unity3d.com/ja/2022.3/ScriptReference/ParticleSystem.Simulate.html

[image1]: assets/kato-yuya-1/media/image1.png

[image2]: assets/kato-yuya-1/media/image2.png

[image3]: assets/kato-yuya-1/media/image3.png