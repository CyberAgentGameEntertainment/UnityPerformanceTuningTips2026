//raw[|latex|\reviewcompactschapterhead]
=[nonum] Introduction

The previous book, Unity Performance Tuning Bible@<fn>{preface_prev_book}, has drawn many responses since its publication in 2022. It explained a workflow for isolating causes, the foundational knowledge behind it, and tuning examples across a wide range of areas from graphics to scripting, so that anyone can learn performance tuning systematically from scratch.

//footnote[preface_prev_book][@<href>{https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningBible}]

In 2026, much of what the previous book describes is still useful. However, four years after its publication, large changes have occurred across a wide range of areas, from the arrival of the Unity 6 generation to changes in the development experience itself brought by AI. To stop and check where performance tuning stands today, we gathered in-house experts and compiled this book, Unity Performance Tuning Tips 2026.

This book covers a wide range of subjects with measured values, from features that have become newly available in Unity 6, such as Render Graph, GraphicsStateCollection, and Link Time Optimization for Android, to practical topics you can try right away in the field.

Tuning decisions ultimately depend on the measurement results of each project. What this book offers is material for deciding where to look, what to compare against, and where to stop. We hope it helps your development, even if only a little.

==[nonum] About This Book

This book assumes smartphone applications. Please note that some explanations may not apply to other platforms.

Unity 6 (6000.x) was used for verification, but the version differs from chapter to chapter. Unless otherwise noted, refer to the version stated in each chapter. Except for chapters covering features added in Unity 6 or later, much of the content also applies to earlier versions. All numbers in this book are measured values or estimates obtained in the measurement environment stated in each chapter. Values and trends change with the environment, so please read what had which effect rather than the numbers themselves.

==[nonum] Structure of This Book

This book consists of 4 parts and 21 chapters. Each chapter is independent, so you can read only the parts you need. The "Problem-Based Reverse Lookup Guide" at the front summarizes typical symptoms and the chapters that cover them. If you already know your problem, looking it up there is the shortest path. The "Prologue" summarizes what has happened to Unity and the mobile environment in the years since the previous book.

Part 1, "Graphics Optimization", covers rendering topics such as saving tile memory with Render Graph, when to use UnsafePass, omitting FinalBlitPass, and shader reduction and warmup.

Part 2, "Scripting and Animation Optimization", covers character animation tuning, reducing bone load with VAT, ParticleSystem spikes, lifetime management of asynchronous processing, and choosing between UI systems.

Part 3, "Asset, Settings, and Build Optimization", covers lifetime management of AssetBundles, investigating asset bloat, switching rendering settings per device, Code Stripping and LTO, and iOS memory limits.

Part 4, "Measurement and Automation", covers iOS standard measurement features, a setup that automates physical-device profiling of daily builds, and an attempt to convert the knowledge in this book into a skill for AI coding agents.

==[nonum] GitHub

The repository for this book@<fn>{preface_repo} is public. We plan to add and revise content from time to time. You can also point out corrections or propose additions through PRs and Issues. Please feel free to use it.

//footnote[preface_repo][@<href>{https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026/}]
