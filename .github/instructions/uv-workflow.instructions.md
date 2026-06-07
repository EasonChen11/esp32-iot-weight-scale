---
description: 'Enforce uv workflow for all Python files'
applyTo: '**/*.py, **.ipynb, pyproject.toml'
---
<!-- ## 2. Python Development Environment -->
# uv Workflow Enforcement

You are working in a modern Python environment managed by `uv`.

- **Package Manager:** Always use **`uv`** as the package manager and virtual environment manager.
- **Package Installation:** Always use `uv add <package_name>` to install packages.
  - **DO NOT** use `pip install`.
- **Virtual Environment:** The project is configured to use a local virtual environment.
  - When executing commands or running scripts in a new terminal, you must assume the environment needs activation.
  - **Activation Command:** `source .venv/bin/activate`
