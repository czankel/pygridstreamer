from setuptools import setup, Extension
import os

gridstreamer_destdir = os.environ.get("GRIDSTREAMER_DESTDIR", "/usr/local")

setup(
    package_data={"pygridstreamer": ["lib/libpygridstreamer.so"]},
    ext_modules=[
        Extension(
            'pygridstreamer',
            include_dirs=[gridstreamer_destdir + "/include", 'source'],
            library_dirs = [gridstreamer_destdir + "/lib"],
            libraries = ["gridstreamer"],
            sources = [
                'source/arguments.cc',
                'source/callback.cc',
                'source/cell.cc',
                'source/channel.cc',
                'source/grid.cc',
                'source/gridmodule.cc',
                'source/parameter.cc',
                ],
            extra_compile_args=["-std=c++17"],
            language = "c++")
        ]
    )
