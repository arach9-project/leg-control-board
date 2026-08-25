Building this documentation
===========================

Create a virtual environment, install the pinned dependencies, and build::

   cd docs-site
   python3 -m venv .venv
   . .venv/bin/activate
   python -m pip install -r requirements.txt
   make html

Open ``build/html/index.html``. The build treats warnings as errors. GitHub
Actions builds pull requests and publishes ``build/html`` to GitHub Pages after
pushes to ``main``. Enable **GitHub Actions** as the repository's Pages source.

