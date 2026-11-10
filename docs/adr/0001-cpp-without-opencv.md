# C++17 with vendored stb, no OpenCV

The engine is written in C++17. Images are read and written with the header-only `stb_image` / `stb_image_write` (vendored in `third_party/`), and Sobel is hand-written. We considered Python and OpenCV: OpenCV is awkward to install under MinGW and adds a heavy dependency for every teammate, and hand-writing Sobel and the energy code keeps the whole pipeline explainable for the team-based quiz and the divide-and-conquer discussion (Q2b).
