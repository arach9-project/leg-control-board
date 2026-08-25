Building this documentation
===========================

The site uses Sphinx and the Furo theme::

   cd docs-site
   python3 -m venv .venv
   . .venv/bin/activate
   python -m pip install -r requirements.txt
   make html

Open ``build/html/index.html``. Warnings fail the build. GitHub Actions checks
pull requests and publishes ``build/html`` to GitHub Pages on ``main``. Enable
**GitHub Actions** as the Pages source in repository settings.

