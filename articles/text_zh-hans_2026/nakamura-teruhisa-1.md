# **将调优宝典转化为 AI 编程智能体的技能**

姓名 / 罗马字: Teruhisa Nakamura  
所属: 株式会社 Applibot

# **前言**

像本书这样的调优知识存在一个结构性弱点：它只对读过的人有效。  
各章都汇集了经过实测验证的经验，但这些经验能否在项目的代码审查中被实际指出，取决于审查者是否读过相应章节，并且能否在审查的那一刻想起来。我们无法期望团队全员背下所有章节，而在运营阶段人员更替后，这些经验也会逐渐流失。  
另一方面，AI 编程智能体（Claude Code、OpenAI Codex、GitHub Copilot 等）已经常驻于编写代码的一线。只要接入审查请求或 CI，就能对所有变更套用同一套标准。所缺的只是一套关于 Unity 性能的系统化评估标准。  
因此本章将介绍如何把调优宝典的知识转换为“AI 智能体可执行的评估项”，并打包为不依赖特定工具的通用技能。笔者实际将基于前作已公开知识的 77 条规则，与从本书（2026 年版）各章稿件中提取的 86 条规则合并，整理为共计 163 条评估规则并制成技能。本章以此案例为基础，总结转换的设计方针与实际使用中的注意事项。

# **Agent Skills 这一通用格式**

## **SKILL.md 的规范**

作为让 AI 智能体具备专业知识的方法，2025 年末公开了名为 Agent Skills{{fn:agentskills}} 的开放规范。技能的实体是以名为 SKILL.md 的 Markdown 文件为核心的目录，结构如下。  
CODE START text | 技能目录的结构 {{list:skill_layout}}  
unity-perf-review/  
├── SKILL.md          \# 元数据 \+ 执行步骤（必需）  
├── references/       \# 规则集等参考文档  
├── scripts/          \# 可执行脚本（可选）  
└── assets/           \# 模板等（可选）  
CODE END  
SKILL.md 的开头要写一段称为 frontmatter 的元数据。必需字段只有 name 和 description 两个。  
CODE START yaml | SKILL.md 的 frontmatter（description 为适应版面已缩短，metadata 等可选字段已省略） {{list:skill_frontmatter}}  
\---  
name: unity-perf-review  
description: \>  
  Unity 项目的代码、资源与设置从性能角度进行审查。  
  将 C\# 脚本、着色器、.meta、ProjectSettings 与  
  163 条规则进行比对，输出 Critical / Warning / Info 级别的问题。  
  Unity 的性能审查或优化调查任务被提出时使用。  
license: MIT  
\---  
CODE END  
关键在于 description 的写法。智能体会将用户的请求内容与 description 进行比对来自动触发技能，因此不仅要写“做什么”，还要写明“何时使用”，这样能提高触发的准确度。

## **主要智能体的支持情况**

这一规范的价值在于多个智能体都能读取同一格式。截至 2026 年 8 月的支持情况见{{table:agent_support}}。  
表: 主要编程智能体对技能的支持情况（截至 2026 年 8 月） {{table:agent_support}}

| 智能体 | 支持情况 | 放置位置 |
| :---- | :---- | :---- |
| Claude Code | 原生支持 SKILL.md{{fn:claudecode}} | .claude/skills/ 或 \~/.claude/skills/ |
| OpenAI Codex | 支持 SKILL.md | .agents/skills/ 或 \~/.agents/skills/ |
| GitHub Copilot | 支持 SKILL.md{{fn:copilot}} | .github/skills/ |
| Gemini CLI | 支持 SKILL.md{{fn:gemini}} | .gemini/skills/ 或 \~/.gemini/skills/ |

Codex / Copilot / Gemini CLI 也都支持通用的 .agents/skills/ 放置位置。此外，Copilot 也会读取项目的 .claude/skills/，因此只需放置到 .claude/skills/ 一处，Claude Code 和 Copilot 两者都能使用（Copilot 的个人放置位置为 \~/.copilot/skills/ 或 \~/.agents/skills/）。支持情况与放置位置随时间变化，引入时请在各智能体的官方文档中确认第一手信息。  
一旦把调优知识落实为 SKILL.md 格式，无论成员使用哪种智能体，都能分发同一套审查标准（规则与步骤）。Agent Skills 统一的是打包格式与步骤，由于模型和工具权限的差异，执行结果并不能完全一致。即便如此，“审查标准放在仓库里，执行交给智能体”这样的分工仍然成立。

# **把知识转换为“评估项”**

技能化的核心工作不是编写 SKILL.md，而是把各章的知识分解到智能体能够机械化套用的粒度。笔者统一采用了以下格式。  
CODE START text | 规则格式（从 ParticleSystem 章节提取的示例。grep\_pattern 的排除条件与对策的细节为适应版面已缩短） {{list:rule_format}}  
\#\#\# US11 向 ParticleSystem.Simulate 传入过大的秒数  
\- 级别: Warning（启用 Noise 模块时为 Critical）  
\- 适用范围: 代码审查 / 资源设置  
\- 检测模式: 向 Simulate 的第 1 个参数传入了较大的秒数。  
  尤其是与启用了 NoiseModule 的 ParticleSystem 搭配使用时  
\- grep\_pattern: \\.Simulate\\(  
\- 理由: Simulate 会一次性模拟到指定时间。  
  实测中指定 20 秒导致 NoiseJob 被调用约 2000 次，  
  产生约 50ms 的尖峰  
\- 对策: 对启用了 Noise 的粒子避免使用 Simulate  
CODE END  
这一格式包含 4 项设计决策。

## **明确检测是否可机械化**

每条规则都以 grep\_pattern 的形式，带有针对 C\# 代码或 Unity 的 YAML（.meta、Prefab、ProjectSettings）的可机械执行的检索规范。大多数是一条正则表达式，但也包含多行匹配或检测“应有描述缺失”的规则。智能体先用这一规范做初次检测，再只对命中位置进行精读与判断。这样即使是大型仓库也能以现实的成本完成审查。  
同样重要的是，不要勉强去写正则表达式无法表达的项目。例如“未注册到 Addressables 的共享资源被多个 Bundle 引用”就无法用 grep 检出。对这类规则要明确标注为（手动确认），并在对策栏写明 Addressables Analyze 等确认手段。如果虚报了可检测性，智能体要么大量产生误报，要么默默跳过无法检测的规则。

## **在理由栏保留实测值**

“理由：因为会导致性能恶化”这样的规则没有说服力。应把稿件中的实测值原样引用到理由栏。

* 未压缩纹理: 2048x2048 时 Compressed 2.7MB / Uncompressed 16.0MB（约 6 倍差距）  
* 每个角色一个 Graph 的手动 Playable: 600 个角色时 8.396ms，整体共用一个 Graph 时 4.500ms（1.87 倍差距）  
* Simulate(20f) 与 Noise 搭配使用: NoiseJob 约 2000 次调用、约 50ms 的尖峰

有了数值，智能体指出的问题本身就成为“为什么应该修改”的说明，也成为接受审查的开发者回到稿件相应章节的入口。需要注意的是，这些都是稿件中在特定环境下的实测值。我们的做法是：规则的理由栏引用代表值，测量条件的细节则参照出处章节。

## **用级别与覆盖机制让规则经得起实际使用**

为规则赋予 Critical / Warning / Info 三个级别（Critical＝必然导致劣化的原因，确认适用条件成立后必须修复；Warning＝视条件可能成为问题的潜在风险；Info＝可选的改进建议），并提供可由项目方覆盖的机制（rules-overrides）。例如即便“使用 LINQ 为 Critical”，也可以调整为在初始化时（init 上下文）放宽为 Warning、在 Editor 工具（tooling）中 Skip。  
调优规则本质上依赖上下文。如果没有覆盖机制，就会出现“不适合我们项目的问题太多”，进而整个技能被禁用。标准统一、应用则由项目各自决定，这种两层结构是长期维持的诀窍。

## **无法静态检测的知识另归为诊断知识**

稿件中的知识有很多是光读代码无法得知的。例如“削减了 CPU 处理但帧时间没有变化，时间转移到了 Semaphore.WaitForSignal 等等待标记上时，应怀疑渲染线程处于关键路径上，并在 Timeline 视图中确认等待的对象”，这类是关于如何解读 Profiler 的知识。仅凭等待标记无法断定瓶颈因素，这一局限也作为知识的一部分保留下来。  
这些知识与评估规则分开，整理为“症状 → 原因 → 对策”的表格放在单独文件中（{{table:diagnosis_example}}）。在笔者的仓库中，只有性能分析用的智能体（面向 Claude Code）会在诊断时读取它。本章分发的技能中将其作为参考资料一并附带，用作把静态审查无法判断的疑点报告为“建议进行性能分析测量”时的依据。  
表: 诊断知识的示例 {{table:diagnosis_example}}

| 症状 | 原因 | 对策 |
| :---- | :---- | :---- |
| CPU 图表呈现谷与尖峰交替的梳状 | 降频执行时忘记错开相位 | 用 index % interval 分散相位 |
| MeshSkinning.CalcMatrices 占比大 | 骨骼 Transform 层级被实体化 | 应用 Optimize Game Objects |
| 仅 iOS 上，没有错误也没有 Shader.CreateGPUProgram，只出现 GpuProgramMetal.GetCachedPipeline | 怀疑着色器在 AssetBundle 中重复打包 | 确认重复实例并拆分为专用 Bundle |
| 仅 iOS 上，预热完成后 SubPass 内的着色器仍发生 PSO 编译 | SubPass（Native Render Pass）内着色器预热失败 | 重新审视 SubPass 或在加载中预渲染 |

# **组装为技能**

## **渐进式加载与清单方式**

若把 163 条规则全部写进 SKILL.md 正文，会挤占智能体的上下文。Agent Skills 的设计思想是渐进式加载（progressive disclosure）。

1. 智能体平时只看 frontmatter 中的 name 和 description  
2. 技能触发后读取 SKILL.md 正文（执行流程）  
3. 只从 references/ 读取需要用到的参考文档

初始阶段读取哪些内容（是否包含路径等）取决于智能体的实现，但“正文与参考资料在触发前不会被读取”这一原则是共通的。  
这里需要注意的是覆盖的完整性。最直观的做法是“按目标文件类型整体读入分类规则文件”，但这种方式下智能体在读代码之前要读的规则最多会超过 2 万 token（按字符数估算，token 数因模型而异）。如果把“记住全部几十条规则并逐一套用”寄托于模型的注意力，部分规则会被悄悄跳过。  
于是，我们从全部规则自动生成一份清单，把“ID / 级别 / 适用条件 / grep\_pattern / 规则名 / 详情文件名”压缩为每条规则一行，并把审查分为两个阶段。

1. **初步扫描**: 随附脚本 scan.py 对目标文件机械执行清单中所有行的 grep\_pattern，输出命中列表（文件:行）与 Coverage（在没有 python3 的环境中，回退为由智能体直接执行所有行的 grep）  
2. **二次核查**: 仅对命中的规则，从详情文件读取检测条件、理由与对策，排除误报

少数无法用 grep 表达的规则在清单上明确标注为 MANUAL，仅当与目标类型相符时才精读。以“应有描述缺失”为报告条件的规则标注为 ABSENT，在 grep 未命中时确认详情（这是为了避免把命中与否和报告条件的对应关系弄反而做的明示）。把初步扫描做成脚本，是因为让 LLM 逐条执行几十个 grep 可能发生遗漏，而且正则表达式的流派差异（如 PCRE 的前瞻在 macOS 自带的 grep 中不可用）会使结果依赖于环境。这样初步扫描的完整性就与模型的记忆力脱钩了。所报问题的正确性（二次核查中的误报排除）仍然由智能体判断。事先读取的只有清单（代码用约 16KB、资源用约 8KB。token 数因模型而异，合计估算在 1 万 token 左右），详情只读命中的部分。清单由规则源文件自动生成，因此即使新增规则也不会产生手动同步的工作。

## **按环境排除规则**

有些规则依赖于 Unity 版本或项目配置。本技能收录的 RenderGraph 相关规则（面向 URP）仅适用于 Unity 6 及之后的版本，LTO 系列规则为 IL2CPP 专用，iOS 特有的 PSO 问题与仅发布 Android 的游戏无关。比对不适用的规则只会带来噪声和额外工作。  
因此，为依赖环境的规则赋予适用条件标签（unity\>=6000.3、urp、il2cpp、ios 等），与根据项目设置文件生成的环境配置文件进行比对，只排除“明确不满足条件的规则”。标签的比对由初步扫描脚本确定性地处理，未能检测出的项目（unknown）按保守方式处理（仍作为比对范围），跳过的规则 ID 会在报告中列出以便审计。生成步骤将在引入步骤一节中说明。

## **强制在输出中附带规则 ID**

在输出格式中，要求每条指出的问题必须附带规则 ID。  
CODE START text | 审查输出的示例 {{list:review_output}}  
\#\#\# Critical  
\- Assets/Scripts/EffectPlayer.cs:42 \-  
  对启用了 Noise 的粒子调用 Simulate(20f) \[US11\]  
  \- 依据: NoiseJob 被调用约 2000 次，产生约 50ms 的尖峰（实测）  
  \- 建议: 移除 Noise 或将秒数降至最小  
CODE END  
有了 ID，就能把所指问题的依据追溯到规则文件进行验证，从而区分“智能体只是说得像那么回事”的情况。即使是误报，也能明确应修改哪条规则的检测模式。

# **分发与向各智能体的放置**

技能的公开只需整个目录共享即可，无论通过 GitHub 仓库还是 zip 都可以。使用者只要复制到{{table:agent_support}}所示的放置位置，各智能体就会发现该技能。  
实际使用上的要点是保持规则的单一来源。在笔者的仓库中，规则源文件只放在 rules/ 目录下，分发用的包由脚本组装（使用者侧的引入步骤见下一节）。  
CODE START bash | 分发包的组装（面向维护者） {{list:packaging}}  
./package-skill.sh  
cp \-r dist/unity-perf-review /path/to/repo/.claude/skills/   \# Claude Code  
cp \-r dist/unity-perf-review /path/to/repo/.agents/skills/   \# Codex / Copilot / Gemini CLI  
cp \-r dist/unity-perf-review /path/to/repo/.github/skills/   \# Copilot 专用存放位置（通过上面两行中的任一行即可被读取到，因此可选）  
CODE END  
一旦开始为每个智能体分别维护规则，每次更新标准都会逐渐偏离。建议一开始就确定“来源只有一处，放置由脚本完成”。

# **引入到自己的项目**

以下是在读者的项目中运行上述机制的步骤。如果已安装 python3，复制与初始生成大约需要 10 分钟。添加到仓库的只有技能目录和环境配置文件（tuning-env.md），不会修改 Unity 的 Assets / ProjectSettings。

## **技能的获取与放置**

技能的全套内容（SKILL.md、附带 163 条规则的 references/、环境检测 / 初步扫描 / 清单重新生成这 3 个脚本，以及说明引入步骤的 README）已在本书仓库的 skills/unity-perf-review/ 下公开{{fn:repo}}。只需获取后复制到所用智能体的技能目录即可。需要注意的是，技能是包含“对智能体的指令与可执行脚本”的分发包。对于来源不可信的技能，建议先确认 SKILL.md 和 scripts/ 的内容再引入，并固定版本。  
CODE START bash | 技能的获取与放置 {{list:install_steps}}  
git clone https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026.git  
cd UnityPerformanceTuningTips2026/skills

\# 根据所用的智能体复制到其中之一  
cp \-r unity-perf-review /path/to/your-repo/.claude/skills/   \# Claude Code  
cp \-r unity-perf-review /path/to/your-repo/.agents/skills/   \# Codex / Copilot / Gemini CLI（通用放置位置）  
cp \-r unity-perf-review /path/to/your-repo/.github/skills/   \# Copilot 专用存放位置（通过上面两行中的任一行即可被读取到，因此可选）  
CODE END  
放置在仓库根目录下，就能向团队全员分发同一套审查标准。如果只是个人试用，放在主目录一侧（如 \~/.claude/skills/）也能运行。

## **放置后的状态**

以 Claude Code 为例，完成放置和下一项的环境配置文件生成后，仓库会呈现如下状态（{{list:installed_layout}}）。  
CODE START text | 放置后的仓库结构 {{list:installed_layout}}  
your-repo/  
|-- .claude/  
|   \`-- skills/  
|       \`-- unity-perf-review/  
|           |-- SKILL.md                      \# name/description \+ 执行流程  
|           |-- README.md                     \# 面向人的引入步骤  
|           |-- references/  
|           |   |-- checklist-code.md         \# 初步扫描用（代码类）  
|           |   |-- checklist-asset.md        \# 初步扫描用（资源/设置类）  
|           |   |-- checklist-profile.md      \# 性能分析用（静态审查中不使用的附属资料）  
|           |   |-- rules-csharp.md 等共 13 个文件  \# 详细规则（共 163 条）  
|           |   |-- diagnosis-knowledge.md    \# 症状-\>原因-\>对策的诊断集  
|           |   \`-- rules-overrides.example.md  
|           \`-- scripts/  
|               |-- detect-env.py             \# 生成环境配置文件  
|               |-- scan.py                   \# 机械执行初步扫描  
|               \`-- gen-checklists.py         \# 修改规则时重新生成清单  
|-- tuning-env.md                             \# 环境配置文件（在下一项中生成）  
\`-- （Unity 项目本身无改动）  
CODE END  
3 份清单按目标分类（代码 / 资源 / 性能分析），一条规则可能同时出现在多份清单中，因此行数合计与规则数 163 并不一致。  
放置完成时，加载到智能体中的原则上只有 frontmatter 的 name 和 description（合计数百 token。如前所述，视实现可能还会加上路径等）。请求审查后，会按照前述的渐进式加载，依次按需读取 SKILL.md 正文 → 与目标对应的清单 → 命中规则的详情。通常无需一次性投入 163 条规则的全文（约 125KB，估算为 4 万多 token）。  
rules-overrides.example.md 原样放置时不会生效。开始进行项目特定的调整时，将其重命名为 rules-overrides.md 后编辑（后述）。

## **生成环境配置文件**

如前所述，163 条规则中包含依赖环境的规则。为了让智能体排除不适用于项目的规则，需要生成环境配置文件。  
CODE START bash | 生成环境配置文件（在 clone 下来的仓库的 skills/ 中执行） {{list:detect_env}}  
python3 unity-perf-review/scripts/detect-env.py \\  
    /path/to/your-repo /path/to/your-repo/tuning-env.md  
CODE END  
若在放置后执行，请将路径替换为放置目标（.claude/skills/ 或 .agents/skills/ 等所用智能体的技能目录）。即使是 Unity 项目不在仓库根目录而位于子目录的 monorepo 结构，只要传入仓库根目录，脚本就会递归查找设置文件并检测。  
执行脚本需要 python3（在没有 python3 的环境中，可以手动创建 tuning-env.md，或跳过此步骤、以比对全部规则的方式使用。初步扫描的 scan.py 也使用 python3，但没有时会回退为由智能体直接执行 grep）。脚本从 ProjectVersion.txt、Packages/manifest.json、GraphicsSettings.asset、ProjectSettings.asset 推测 Unity 版本、渲染管线、Scripting Backend 以及是否使用 Addressables。这终究只是基于设置文件的推测值，确定信息以手动填写到 tuning-env.md 的内容为准。未能检测出的项目（例如按平台分别设置的 Scripting Backend 混用时）会成为 unknown，按保守方式处理（仍作为比对范围）。请打开输出的 tuning-env.md 确认内容，尤其要手动修正发布平台和是否使用 SideFX VAT，并确认 render\_pipeline 是否为预期的值。

## **项目特定的调整（可选）**

如果存在有意偏离规则的地方，就在 rules-overrides.md 中记录级别的覆盖。例如允许初始化时使用 LINQ、在 Editor 工具中禁用日志规则之类的调整。环境配置文件负责排除“在该环境中原理上不适用”的规则，overrides 负责“出于项目考虑的调整”，两者各司其职。  
如果想新增或修改规则本身，编辑 references/ 下的 rules-\*.md，然后执行 python3 scripts/gen-checklists.py references/ 重新生成清单。清单是自动生成物，不要直接编辑。仅凭分发包就能完成这一新增与重新生成的循环。

## **执行**

只需向智能体发出请求即可。通常会根据请求文本与 description 的匹配自动触发（触发由模型判断，若想确保使用，请在请求文本中明确写出技能名 unity-perf-review，或使用各智能体的显式调用方式）。  
CODE START text | 审查请求的示例 {{list:invoke_example}}  
从性能角度审查这份变更  
Assets/Scripts/Battle/ 用调优宝典的规则检查该目录下的内容  
CODE END  
在输出末尾附带的 Coverage 部分，可以确认比对的规则数、因环境跳过的规则数（附 ID）以及 grep 命中数。这些数量直接摘自初步扫描脚本的输出，不依赖智能体的记忆（只有二次核查中采纳与误报排除的数量由智能体统计）。如果觉得排除过多，请调整 tuning-env.md；如果觉得报告的问题过多，请调整 rules-overrides.md。

# **实际使用中的注意事项**

以下列举实际引入项目后发现的注意点。  
**首次运行不要从整个仓库开始，而是从变更或特定目录开始。** 初步扫描本身即使针对整个项目也能在短时间内完成，但在有运营历史的代码库中，命中的规则会达到几十条的规模，二次核查（排除误报）的负担会集中在此。先在较小范围内核查，把长期允许的项目落实到 overrides 后再扩大范围，指出的问题才不会被噪声淹没，更容易落地。  
**静态分析也会对有意为之的设置报告问题。** 例如有意不压缩的纹理也会被 AS02（未设置纹理压缩）命中。不要盲信检测结果批量修改，而应逐项确认意图后再修改。将长期允许的项目连同理由记录到 overrides，可以减少之后的噪声。overrides 的调整单位是整条规则或上下文（in-game＝热路径 / init＝初始化 / tooling＝Editor），仅针对特定资源的例外则在备注栏写上路径和理由，作为智能体的判断材料（当前版本尚不支持专门的适用范围指定）。  
**实测值带有其测量环境。** 规则中引用的数值是在特定 Unity 版本、特定设备上的实测结果。不能把“在 Pixel 8a 上的实测”外推到最新高端设备的判断上。应在规则本身写明适用版本，并把 Unity 或渲染管线的更新作为重新审视规则的触发点。  
**最终做出判断的是人。** 借用本书另一章的说法，“制定与修订规约的是人，让人遵守规约的是机器”。AI 智能体是“让人遵守的机器”的新实现，而对规约＝评估项的合理性负责的仍然是人。如果无法认同智能体指出的问题，请把它视为改进该规则的检测模式或理由栏的信号。

# **结语**

尝试把调优宝典的知识规则化后，切身体会到“实测值与对策成对写出的稿件，几乎可以原样成为机器可读的评估项”。反过来说，没有数值依据的经验法则会在规则化阶段暴露出弱点。这也是对写作方的反馈。写明测量环境与数值的知识，不仅能传达给本书的读者，还能通过 AI 智能体传达到所有代码审查中。  
本章介绍的技能与规则集与其说是唯一的正确答案，不如说是一个起点。期待它成为一份活的规约集：各项目的 overrides 不断成长，新章节的知识不断作为规则加入。  
脚注: {{fn:repo}} https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026/tree/main/skills/unity-perf-review  
脚注: {{fn:agentskills}} Agent Skills 规范 https://agentskills.io/specification  
脚注: {{fn:claudecode}} Claude Code 的 Skills 文档 https://code.claude.com/docs/en/skills  
脚注: {{fn:copilot}} GitHub Docs "About agent skills" https://docs.github.com/en/copilot/concepts/agents/about-agent-skills  
脚注: {{fn:gemini}} Gemini CLI 的 Agent Skills 文档 https://geminicli.com/docs/cli/using-agent-skills/