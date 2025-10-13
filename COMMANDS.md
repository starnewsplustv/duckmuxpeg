# Command Index

This document tracks the shell commands executed while working on this task.

1. `ls`
2. `cd duckmuxpeg`
3. `ls`
4. `find .. -name AGENTS.md -print`
5. `ls src`
6. `cat README.md`
7. `rg "TODO" -n`
8. `cat CMakeLists.txt`
9. `ls src/obs-ui`
10. `cat src/obs-ui/BroadcastComplianceUI.h`
11. `cat src/obs-ui/CMakeLists.txt`
12. `ls src/common`
13. `cat build.sh`
14. `wc -l build.sh`
15. `sed -n '120,260p' build.sh`
16. `ls src/broadcast`
17. `cat src/broadcast/ATSCCompliance.h`
18. `cat src/broadcast/CMakeLists.txt`
19. `ls src/encoder`
20. `cat src/encoder/CMakeLists.txt`
21. `ls src/statmux`
22. `sed -n '1,200p' src/main.cpp`
23. `sed -n '1,160p' src/common/Logger.cpp`
24. `rg "RateController" -n`
25. `sed -n '1,200p' src/common/Config.h`
26. `sed -n '1,200p' src/common/Config.cpp`
27. `sed -n '200,400p' src/common/Config.cpp`
28. `sed -n '1,200p' src/encoder/ModernMPEG2Encoder.h`
29. `sed -n '200,400p' src/encoder/ModernMPEG2Encoder.h`
30. `sed -n '1,200p' src/encoder/LookaheadAnalyzer.h`
31. `sed -n '1,200p' src/encoder/ParallelEncoder.h`
32. `sed -n '200,400p' src/encoder/ParallelEncoder.h`
33. `rg "FrameAnalysis" -n src`
34. `rg "ParallelEncoder" -n src`
35. `sed -n '160,320p' src/broadcast/ATSCCompliance.h`
36. `sed -n '320,480p' src/broadcast/ATSCCompliance.h`
37. `sed -n '1,200p' src/encoder/RAMBufferManager.h`
38. `rg "RAMBufferManager" -n`
39. `sed -n '1,200p' src/common/CircularBuffer.h`
40. `cat src/obs-plugin/CMakeLists.txt`
41. `cat src/common/CMakeLists.txt`
42. `rg "\\.\\.\\." build.sh`
43. `cat <<'EOF' > COMMANDS.md`
44. `cat <<'EOF' > CMakeLists.txt`
45. `rm src/encoder/LookaheadAnalyzer.h src/encoder/ParallelEncoder.h src/encoder/RAMBufferManager.h`
46. `cat <<'EOF' > src/encoder/ModernMPEG2Encoder.h`
47. `apply_patch <<'EOF'` (ModernMPEG2Encoder.h update)
48. `cat <<'EOF' > src/encoder/ModernMPEG2Encoder.cpp`
49. `cat <<'EOF' > src/encoder/CMakeLists.txt`
50. `cat <<'EOF' > src/broadcast/ATSCCompliance.h`
51. `cat <<'EOF' > src/broadcast/ATSCCompliance.cpp`
52. `cat <<'EOF' > src/broadcast/CMakeLists.txt`
53. `cat <<'EOF' > src/obs-ui/BroadcastComplianceUI.h`
54. `cat <<'EOF' > src/obs-ui/BroadcastComplianceUI.cpp`
55. `cat <<'EOF' > src/obs-ui/CMakeLists.txt`
56. `cat <<'EOF' > install.sh`
57. `chmod +x install.sh`
58. `tail -n 40 build.sh`
59. `printf '\n' >> build.sh`
60. `tail -n 3 build.sh`
61. `apply_patch <<'EOF'` (main.cpp dashboard integration)
62. `apply_patch <<'EOF'` (main.cpp include <vector>)
63. `sed -n '60,140p' README.md`
64. `apply_patch <<'EOF'` (README updates)
65. `cmake -S . -B build`
66. `git status -sb`
67. `rm -rf build`
68. `git add ...`
69. `git rm src/encoder/LookaheadAnalyzer.h src/encoder/ParallelEncoder.h src/encoder/RAMBufferManager.h`
70. `git status -sb`
71. `git commit -m "Implement encoder, compliance UI, and install tooling"`
