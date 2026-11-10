# Native Dear ImGui + ImPlot desktop frontend

The interactive frontend (bonus b) is a native desktop app using Dear ImGui and ImPlot on GLFW + OpenGL3, with benchmark charts shown in-app and exportable as CSV. We rejected a local web server (extra HTML/JS surface) and a Python/Streamlit wrapper (a second language the quiz and report would have to cover). The engine stays in `seamlib` with no GUI dependency, so the same code serves the headless `seamcli` used by tests and CI.
