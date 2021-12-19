from distutils.core import setup

setup(
    name='erpc_tinyproto',
    author='Lijun Chen',
    author_email='chenlijun1999@gmail.com',
    url='https://github.com/Kerr-srl/erpc-esp/tree/dev/src/components/erpc_tinyproto',
    license='MIT',
    version='0.0.1',
    description='Tinyproto transport for eRPC',
    package_dir={"erpc_tinyproto": "./"},
    packages=['erpc_tinyproto'],
)
