from conans import CMake, ConanFile, tools
from conans.errors import ConanException


class GerberaConan(ConanFile):
    name = "gerbera"
    license = "GPLv2"

    generators = ("cmake", "cmake_find_package")
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "js": [True, False],
        "debug_logging": [True, False],
        "tests": [True, False],
        "magic": [True, False],
        "curl": [True, False],
    }
    default_options = {
        "js": True,
        "debug_logging": False,
        "tests": False,
        "magic": True,
        "curl": True,
    }

    scm = {"type": "git"}

    requires = [
        "fmt/6.2.1",
        "spdlog/1.6.0",
        "pugixml/1.10@bincrafters/stable",
        "libuuid/1.0.3",
        "libiconv/1.16",
        "sqlite3/3.31.1",
        "zlib/1.2.11",
        "pupnp/1.12.1",
    ]

    def configure(self):
        tools.check_min_cppstd(self, "17")
        if self.options.tests:
            # We have our own main function,
            # Moreover, if "shared" is True then main is an .so...
            self.options["gtest"].no_main = True

    def requirements(self):
        if self.options.tests:
            self.requires("gtest/1.10.0")

    def system_requirements(self):
        os_info = tools.OSInfo()
        if os_info.with_apt:
            magic_pkg = "libmagic-dev"
            curl_pkg = "libcurl4-openssl-dev"
        elif os_info.with_pacman:
            magic_pkg = "file-dev"
            curl_pkg = "curl-dev"
        elif os_info.with_yum:
            magic_pkg = "file-devel"
            curl_pkg = "libcurl-devel"
        else:
            self.output.warn("Don't know how to install packages.")
            return

        installer = tools.SystemPackageTool(conanfile=self)
        if self.options.magic:
            installer.install(magic_pkg)

        # Note: there is a CURL Conan package, but it depends on openssl
        # which is also in Conan.
        if self.options.curl:
            installer.install(curl_pkg)

    def build(self):
        cmake = CMake(self)
        cmake.definitions["WITH_JS"] = self.options.js
        cmake.definitions["WITH_DEBUG"] = self.options.debug_logging
        cmake.definitions["WITH_TESTS"] = self.options.tests
        cmake.definitions["WITH_MAGIC"] = self.options.magic
        cmake.definitions["WITH_CURL"] = self.options.curl
        cmake.configure()
        cmake.build()
