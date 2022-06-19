from distutils.core import setup
from setuptools import find_packages


setup(
    name="erpc_esp",
    author="Lijun Chen",
    author_email="chenlijun1999@gmail.com",
    url="https://github.com/Kerr-srl/erpc-esp/tree/dev/src/erpc_esp",
    license="MIT",
    version="0.0.1",
    description="eRPC on ESP32",
    package_dir={"": "src"},
    packages=find_packages(
        where="src/",
    ),
)
