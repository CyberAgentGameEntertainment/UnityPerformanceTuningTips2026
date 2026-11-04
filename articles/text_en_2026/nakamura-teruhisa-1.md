# **Turning the Tuning Bible into a Skill for AI Coding Agents**

Name: Teruhisa Nakamura  
Affiliation: Applibot, Inc.

# **Introduction**

Tuning knowledge such as that in this book has one structural weakness: it only helps people who have read it.  
Each chapter is full of findings backed by measurements, but whether they are actually raised in a project's code review depends on whether the reviewer has read the relevant chapter and can recall it at the moment of review. You cannot expect every team member to memorize every chapter, and when people rotate during the operation phase, the knowledge fades.  
Meanwhile, AI coding agents (Claude Code, OpenAI Codex, GitHub Copilot, and others) are now a permanent presence where code is written. If you build them into review requests or CI, the same standard can be applied to every diff. The only missing piece is a systematic set of evaluation criteria for Unity performance.  
This chapter therefore shows how to convert the knowledge in the Tuning Bible into "evaluation items that an AI agent can execute" and package it as a general-purpose skill that does not depend on a specific tool. Based on the author's actual case of combining 77 rules built on the knowledge published in the previous book with 86 rules extracted from the manuscripts of this book (2026 edition), for a total of 163 evaluation rules packaged as a skill, it summarizes design guidelines for the conversion and operational considerations.

# **Agent Skills as a Common Format**

## **The SKILL.md Specification**

As a way to give AI agents specialized knowledge, an open specification called Agent Skills{{fn:agentskills}} was released in late 2025. A skill is a directory centered on a Markdown file named SKILL.md, with a structure like the following.  
CODE START text | Skill directory structure {{list:skill_layout}}  
unity-perf-review/  
├── SKILL.md          \# Metadata \+ execution steps (required)  
├── references/       \# Reference documents such as rule sets  
├── scripts/          \# Executable scripts (optional)  
└── assets/           \# Templates, etc. (optional)  
CODE END  
The top of SKILL.md contains metadata called front matter. Only two fields are required: name and description.  
CODE START yaml | SKILL.md front matter (description is shortened for print, and optional fields such as metadata are omitted) {{list:skill_frontmatter}}  
\---  
name: unity-perf-review  
description: \>  
  Unity Reviews the project's code, assets, and settings from a performance perspective.  
  Checks C\# scripts, shaders, .meta files, and ProjectSettings against  
  163 the rules and outputs Critical / Warning / Info findings.  
  Unity Use when asked for a performance review or optimization investigation of  
license: MIT  
\---  
CODE END  
How you write the description is important. The agent matches the user's request against the description to trigger the skill automatically, so writing not only "what it does" but also "when to use it" improves triggering accuracy.

## **Support in Major Agents**

The value of this specification is that multiple agents can read the same format. {{table:agent_support}} shows the support status as of August 2026.  
表: Skill support in major coding agents (as of August 2026) {{table:agent_support}}

| Agent | Support | Location |
| :---- | :---- | :---- |
| Claude Code | Native SKILL.md support{{fn:claudecode}} | .claude/skills/ or \~/.claude/skills/ |
| OpenAI Codex | SKILL.md supported | .agents/skills/ or \~/.agents/skills/ |
| GitHub Copilot | SKILL.md supported{{fn:copilot}} | .github/skills/ |
| Gemini CLI | SKILL.md supported{{fn:gemini}} | .gemini/skills/ or \~/.gemini/skills/ |

Codex, Copilot, and Gemini CLI also support the shared .agents/skills/ location. Copilot also reads the project's .claude/skills/, so a single placement in .claude/skills/ reaches both Claude Code and Copilot (Copilot's personal locations are \~/.copilot/skills/ and \~/.agents/skills/). Support status and locations change over time, so check each agent's official documentation for primary information when adopting the skill.  
Once tuning knowledge is captured in SKILL.md format, you can distribute the same review criteria (rules and procedures) to members who use any agent. Agent Skills standardizes the package format and procedures, but differences in models and tool permissions mean the execution results are not identical. Even so, a division of labor holds: "review criteria live in the repository, execution is done by the agent."

# **Converting Findings into Evaluation Items**

The core of building a skill is not writing SKILL.md but breaking the findings of each chapter down to a granularity that an agent can apply mechanically. The author standardized on the following format.  
CODE START text | Rule format (an example extracted from the ParticleSystem chapter; the exclusion conditions of grep\_pattern and the details of the remedy are shortened for print) {{list:rule_format}}  
\#\#\# US11 Passing a large number of seconds to ParticleSystem.Simulate  
\- Level: Warning (Critical when the Noise module is enabled)  
\- Target: code review / asset settings  
\- Detection pattern: a large number of seconds is passed as the first argument of Simulate.  
  Especially when used with a ParticleSystem that has NoiseModule enabled  
\- grep\_pattern: \\.Simulate\\(  
\- Reason: Simulate simulates in one batch up to the specified time.  
  In measurements, specifying 20 seconds called NoiseJob about 2000 times,  
  causing a spike of about 50 ms  
\- Countermeasure: avoid using Simulate on particles with Noise enabled  
CODE END  
This format embodies four design decisions.

## **Make the Feasibility of Mechanical Detection Explicit**

Each rule has a grep\_pattern, a machine-executable search specification for C\# code and Unity YAML (.meta, Prefab, ProjectSettings). Most are a single regular expression, but some match across multiple lines or detect the absence of a required statement. The agent performs primary detection with this specification and reads only the hit locations closely to make a judgment. This makes review feasible at a realistic cost even for large repositories.  
It is equally important not to force rules into a regular expression when they cannot be expressed as one. For example, "a shared asset not registered in Addressables is referenced from multiple bundles" cannot be found with grep. Such rules are explicitly marked (manual check), and the remedy field lists a means of verification such as Addressables Analyze. If detectability is misrepresented, the agent either produces many false positives or silently skips rules it cannot detect.

## **Keep Measured Values in the Reason Field**

A rule whose reason reads "Reason: performance degrades" is not persuasive. Quote the measured values from the manuscripts directly in the reason field.

* Uncompressed texture: at 2048x2048, Compressed is 2.7MB versus Uncompressed 16.0MB (about 6x difference)  
* Manual Playable with one graph per character: 8.396ms for 600 characters versus 4.500ms for a single graph for all (1.87x difference)  
* Combining Simulate(20f) with Noise: a spike of about 50ms from roughly 2000 NoiseJob calls

With numbers, the agent's finding itself explains "why this should be fixed." It also gives the developer who receives the review a path back to the relevant chapter of the manuscript. Note that all of these are values measured in specific environments as described in the manuscripts. The reason field quotes representative values, and the measurement conditions are left to the source chapter.

## **Make It Sustainable with Levels and Overrides**

Rules are assigned one of three levels: Critical / Warning / Info (Critical = a definite cause of degradation, which must be fixed once its applicability conditions are confirmed; Warning = a latent risk that becomes a problem under certain conditions; Info = an optional improvement recommendation). A mechanism for projects to override them (rules-overrides) is also provided. For example, even if "LINQ usage is Critical," it can be relaxed to Warning during initialization (init context) and set to Skip in Editor tools (tooling).  
Tuning rules are inherently context-dependent. Without an override mechanism, complaints such as "many findings do not suit our project" lead to the whole skill being disabled. A two-layer structure, with common criteria and project-specific application, is the key to longevity.

## **Separate Findings That Cannot Be Detected Statically into Diagnostic Knowledge**

Many findings in the manuscripts cannot be learned from reading the code. One example is knowledge about how to read the profiler: "If you reduced CPU processing but the frame time did not change and the time moved to a wait marker such as Semaphore.WaitForSignal, suspect that the render thread is on the critical path, and check what it is waiting on in the Timeline view." The limitation that a wait marker alone cannot establish the bottleneck is also retained as part of the finding.  
These are separated from the evaluation rules into a different file as a "symptom → cause → remedy" table ({{table:diagnosis_example}}). In the author's repository, a profile analysis agent (for Claude Code) loads it only during diagnosis. The skill distributed in this chapter bundles it as reference material, and it serves as the basis for reporting suspicions that static review cannot judge as "profiling recommended."  
表: Example of diagnostic knowledge {{table:diagnosis_example}}

| Symptom | Cause | Remedy |
| :---- | :---- | :---- |
| CPU chart shows a comb pattern of valleys and spikes | Forgot to offset the phase of throttled (every-N-frame) updates | Distribute the phase with index % interval |
| MeshSkinning.CalcMatrices is large | Bone Transform hierarchy is materialized | Apply Optimize Game Objects |
| iOS only: no errors and no Shader.CreateGPUProgram, but only GpuProgramMetal.GetCachedPipeline occurs | Suspect duplicate packing of shaders in AssetBundles | Check for duplicate instances and move the shaders to a dedicated bundle |
| iOS only: PSO compilation occurs in a SubPass shader even after warmup completes | Warmup failure for shaders inside a SubPass (Native Render Pass) | Review the SubPass usage, or pre-render during loading |

# **Assembling the Skill**

## **Progressive Disclosure and the Checklist Approach**

Writing all 163 rules in the body of SKILL.md would put pressure on the agent's context. The design philosophy of Agent Skills is progressive disclosure.

1. The agent always sees only the name and description in the front matter  
2. When the skill is triggered, it reads the body of SKILL.md (the execution flow)  
3. It reads only the reference documents it needs from references/

What is read initially (including whether paths are included) depends on the agent implementation, but the principle that "the body and reference materials are not read until triggering" is common to all.  
Completeness needs attention here. A naive approach would be to load entire category-specific rule files according to the target file type, but with this approach the rules the agent reads before the code can, in the worst case, exceed 20,000 tokens (an estimate from character count; the token count depends on the model). If you rely on the model's attention to "remember and apply all" of dozens of rules, some of them will be silently skipped.  
The author therefore automatically generated a checklist that compresses every rule into one line with "ID / level / applicability condition / grep\_pattern / rule name / detail file name," and split the review into two stages.

1. **Primary scan**: The bundled script scan.py mechanically runs the grep\_pattern of every checklist line against the target files and outputs a list of hits (file:line) and Coverage (in environments without python3, the agent falls back to running every grep line directly)  
2. **Secondary review**: Only for rules that hit, read the detection conditions, reason, and remedy from the detail file and exclude false positives

Some rules that grep cannot express are marked MANUAL on the checklist and are read closely only when they apply to the target type. Rules where the absence of a required statement is the condition for a finding are marked ABSENT, and their details are checked when grep produces no hit (this is explicit so that the correspondence between hits and finding conditions is not inverted). The primary scan was made a script because having an LLM run dozens of greps one by one can lead to skipped items, and differences in regular expression dialects (for example, PCRE lookahead does not work in the standard macOS grep) make results environment-dependent. This decouples the completeness of the primary scan from the model's memory. The correctness of findings (excluding false positives in the secondary review) remains the agent's judgment. Only the checklists are read in advance (about 16KB for code and about 8KB for assets; the token count depends on the model, but roughly 10,000 tokens in total), and details are read only for the hits. Because the checklists are generated automatically from the rule bodies, adding a rule requires no manual synchronization.

## **Excluding Rules by Environment**

Some rules depend on the Unity version or configuration. The RenderGraph-related rules included in this skill (for URP) target Unity 6 and later, the LTO rules are for IL2CPP only, and iOS-specific PSO issues are irrelevant to titles that ship only on Android. Checking rules that do not apply only adds noise and effort.  
Environment-dependent rules therefore carry applicability condition tags (unity\>=6000.3, urp, il2cpp, ios, and so on), which are matched against an environment profile generated from the project's configuration files, excluding "only the rules that clearly do not meet the conditions." The primary scan script handles tag matching deterministically, items that could not be detected (unknown) are treated conservatively (the rule remains a target for checking), and the IDs of skipped rules are listed in the report so that it can be audited. The generation procedure is described in the section on the adoption steps.

## **Enforce Rule IDs in the Output**

The output format requires every finding to carry a rule ID.  
CODE START text | Example of review output {{list:review_output}}  
\#\#\# Critical  
\- Assets/Scripts/EffectPlayer.cs:42 \-  
  Simulate(20f) call on a particle with Noise enabled \[US11\]  
  \- Evidence: NoiseJob is called about 2000 times, causing a spike of about 50 ms (measured)  
  \- Recommendation: remove Noise or minimize the number of seconds  
CODE END  
With IDs, you can trace the basis of a finding back to the rule file and verify it, which distinguishes it from the agent "just sounding plausible." When a finding is a false positive, it is also clear which rule's detection pattern should be fixed.

# **Distribution and Placement for Each Agent**

Publishing a skill requires only sharing the directory. A GitHub repository or a zip file both work. Users copy it to the locations shown in {{table:agent_support}}, and each agent discovers the skill.  
The operational key is to keep a single source for the rules. In the author's repository, the rule bodies are kept only in the rules/ directory, and the distribution package is assembled by a script (the user-side adoption steps are in the next section).  
CODE START bash | Assembling the distribution package (for maintainers) {{list:packaging}}  
./package-skill.sh  
cp \-r dist/unity-perf-review /path/to/repo/.claude/skills/   \# Claude Code  
cp \-r dist/unity-perf-review /path/to/repo/.agents/skills/   \# Codex / Copilot / Gemini CLI  
cp \-r dist/unity-perf-review /path/to/repo/.github/skills/   \# Copilot-specific placement (optional, since either of the two lines above is sufficient)  
CODE END  
Once you start managing rules separately for each agent, the criteria drift apart with every update. I recommend deciding at the outset on "one source, placement by script."

# **Adopting It in Your Own Project**

This section describes the steps to run the mechanism described so far in your own project. If python3 is installed, copying and initial generation take about 10 minutes. Only the skill directory and the environment profile (tuning-env.md) are added to the repository, and nothing in the Unity Assets / ProjectSettings is changed.

## **Obtaining and Placing the Skill**

The complete skill (SKILL.md, references/ containing the 163 rules, three scripts for environment detection, primary scan, and checklist regeneration, and a README with the adoption steps) is published in skills/unity-perf-review/ of this book's repository{{fn:repo}}. Simply obtain it and copy it to the skill directory of the agent you use. Note that a skill is a distributable containing "instructions for the agent and executable scripts." For skills from untrusted sources, check the contents of SKILL.md and scripts/ before adopting them, and pin the version.  
CODE START bash | Obtaining and placing the skill {{list:install_steps}}  
git clone https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026.git  
cd UnityPerformanceTuningTips2026/skills

\# Copy to one of these, depending on the agent you use  
cp \-r unity-perf-review /path/to/your-repo/.claude/skills/   \# Claude Code  
cp \-r unity-perf-review /path/to/your-repo/.agents/skills/   \# Codex / Copilot / Gemini CLI (shared placement)  
cp \-r unity-perf-review /path/to/your-repo/.github/skills/   \# Copilot-specific placement (optional, since either of the two lines above is sufficient)  
CODE END  
If you place it at the repository root, the same review criteria are distributed to the whole team. If you are only trying it individually, it also works in the home directory (such as \~/.claude/skills/).

## **State After Placement**

For Claude Code, once placement and the environment profile generation in the next section are done, the repository looks like this ({{list:installed_layout}}).  
CODE START text | Repository structure after placement {{list:installed_layout}}  
your-repo/  
|-- .claude/  
|   \`-- skills/  
|       \`-- unity-perf-review/  
|           |-- SKILL.md                      \# name/description \+ execution flow  
|           |-- README.md                     \# Setup instructions for humans  
|           |-- references/  
|           |   |-- checklist-code.md         \# For primary scan (code)  
|           |   |-- checklist-asset.md        \# For primary scan (assets/settings)  
|           |   |-- checklist-profile.md      \# For profile analysis (supplementary material not used in static review)  
|           |   |-- rules-csharp.md 13 files in total, etc.  \# Detailed rules (163 in total)  
|           |   |-- diagnosis-knowledge.md    \# Diagnostic collection of symptom-\>cause-\>remedy  
|           |   \`-- rules-overrides.example.md  
|           \`-- scripts/  
|               |-- detect-env.py             \# Environment profile generation  
|               |-- scan.py                   \# Automated execution of the primary scan  
|               \`-- gen-checklists.py         \# Regenerating the checklist when rules are modified  
|-- tuning-env.md                             \# Environment profile (generated in the next section)  
\`-- (The Unity project itself is not modified)  
CODE END  
The three checklists are by target (code / assets / profile), and one rule can appear on more than one of them, so the total number of lines does not match the 163 rules.  
At the time of placement, in principle only the name and description in the front matter are loaded into the agent (a few hundred tokens in total; as noted earlier, some implementations add paths and the like). When you request a review, the necessary parts are loaded in the following order according to the progressive disclosure described earlier: the body of SKILL.md, then the checklist for the target, then the details of the rules that hit. Normally you do not need to load the full text of all 163 rules at once (about 125KB, roughly over 40,000 tokens).  
rules-overrides.example.md has no effect as it is. When you start making project-specific adjustments, rename it to rules-overrides.md and edit it (described later).

## **Generating the Environment Profile**

As described earlier, the 163 rules include environment-dependent ones. To let the agent decide which rules to exclude because they do not apply to the project, generate an environment profile.  
CODE START bash | Generate the environment profile (run from skills/ in the cloned repository) {{list:detect_env}}  
python3 unity-perf-review/scripts/detect-env.py \\  
    /path/to/your-repo /path/to/your-repo/tuning-env.md  
CODE END  
If you run it after placement, replace the paths with the placement location (the skill directory of the agent you use, such as .claude/skills/ or .agents/skills/). Even in a monorepo where the Unity project is in a subdirectory rather than at the repository root, passing the repository root makes the script search the configuration files recursively and detect them.  
Running the scripts requires python3 (in environments without it, you can create tuning-env.md manually, or skip this step and operate by checking all rules. The primary scan script scan.py also uses python3, but without it the agent falls back to running grep directly). The script estimates the Unity version, render pipeline, Scripting Backend, and presence of Addressables from ProjectVersion.txt, Packages/manifest.json, GraphicsSettings.asset, and ProjectSettings.asset. These are only estimates from the configuration files, and the authoritative information is what you enter manually in tuning-env.md. Items that could not be detected (for example, when the Scripting Backend differs by platform) become unknown and are treated conservatively (the rule remains a target for checking). Open the generated tuning-env.md and check its contents; in particular, manually correct the shipping platforms and whether SideFX VAT is used, and confirm that render\_pipeline has the expected value.

## **Project-Specific Adjustments (Optional)**

If some places intentionally deviate from the rules, record level overrides in rules-overrides.md. Examples are allowing LINQ during initialization or disabling the logging rule in Editor tools. The division of roles is that the environment profile handles excluding rules that are "fundamentally inapplicable in that environment," while overrides handle "adjustments for project circumstances."  
To add or modify rules themselves, edit rules-\*.md in references/ and regenerate the checklists with python3 scripts/gen-checklists.py references/. The checklists are generated files, so do not edit them directly. This cycle of adding and regenerating can be completed with the distributed package alone.

## **Running It**

Simply ask the agent. Normally the skill is triggered automatically when the request matches the description (triggering is a model decision, so to ensure it is used, state the skill name unity-perf-review in the request or use each agent's explicit invocation).  
CODE START text | Example review requests {{list:invoke_example}}  
Review this diff from a performance perspective  
Assets/Scripts/Battle/ Check everything under this path against the Tuning Bible rules  
CODE END  
The Coverage section at the end of the output shows the number of rules checked, the number of rules skipped because of the environment (with IDs), and the number of grep hits. These counts are transcribed from the output of the primary scan script and do not depend on the agent's memory (only the counts of findings adopted and false positives excluded in the secondary review are tallied by the agent). If you feel too many rules are excluded, adjust tuning-env.md; if there are too many findings, adjust rules-overrides.md.

# **Operational Considerations**

These are points I noticed from actually adopting the skill in a project.  
**For the first run, start with a diff or a specific directory rather than the whole repository.** The primary scan finishes quickly even on a whole project, but in a codebase with a long operating history, dozens of rules can hit, and the load of the secondary review (excluding false positives) concentrates there. Reviewing a narrow scope, moving permanently accepted items into overrides, and then widening the target makes findings take hold without being buried in noise.  
**Static analysis also flags intended settings.** For example, a texture that is intentionally left uncompressed also hits AS02 (texture compression not set). Do not take the detection results at face value and fix everything in bulk; confirm the intent one item at a time before fixing. Recording permanently accepted items in overrides with a reason reduces noise from the next run onward. The unit of adjustment in overrides is the whole rule or a context (in-game = hot path / init = initialization / tooling = Editor); for exceptions limited to specific assets, write the path and reason in the notes field as input for the agent's judgment (a dedicated scope specification is not supported in the current version).  
**Measured values come with a measurement environment.** The numbers quoted in the rules are measurements on a specific Unity version and device. A measurement on a Pixel 8a cannot be extrapolated to decisions about the latest high-end devices. Write the target version in the rule itself, and use updates to Unity or the render pipeline as triggers for reviewing rules.  
**The final judgment rests with people.** To borrow the words of another chapter of this book, "people decide and revisit the conventions, and machines enforce them." An AI agent is a new implementation of the "machine that enforces," and humans remain responsible for the validity of the conventions, that is, the evaluation items. If you disagree with an agent's finding, treat it as a signal to improve the rule's detection pattern or reason field.

# **Conclusion**

What I realized from turning the findings of the Tuning Bible into rules is that "a manuscript that states measured values together with remedies becomes a machine-readable evaluation item almost as it is." Conversely, a rule of thumb without supporting numbers shows its weakness at the rule-writing stage. This is also feedback for the writing side. Findings that state the measurement environment and numbers reach not only the readers of the book but every code review through AI agents.  
The skill and rule set introduced in this chapter are a starting point rather than a single correct answer. I hope they grow into a living set of conventions, as each project's overrides mature and findings from new chapters are added as rules.  
脚注: {{fn:repo}} https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026/tree/main/skills/unity-perf-review  
脚注: {{fn:agentskills}} Agent Skills specification https://agentskills.io/specification  
脚注: {{fn:claudecode}} Claude Code Skills documentation https://code.claude.com/docs/en/skills  
脚注: {{fn:copilot}} GitHub Docs "About agent skills" https://docs.github.com/en/copilot/concepts/agents/about-agent-skills  
脚注: {{fn:gemini}} Gemini CLI Agent Skills documentation https://geminicli.com/docs/cli/using-agent-skills/