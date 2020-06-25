import pathlib

from setuptools import setup

HERE = pathlib.Path(__file__).parent
README = (HERE / "README.md").read_text()

setup(name='maxosc',
      version='0.0.3',
      description='Call python code from MaxMSP over OSC',
      long_description=README,
      long_description_content_type="text/markdown",
      url='https://github.com/jobor019/pyosc',
      author='Joakim Borg',
      author_email='joakim.borg@ircam.fr',
      license='MIT',
      packages=['maxosc'],
      install_requires=["python-osc"],
      zip_safe=False)
