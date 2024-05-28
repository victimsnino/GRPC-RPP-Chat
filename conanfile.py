from conan import ConanFile

class Conan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    extension_properties = {"compatibility_cppstd": False}

    def requirements(self):
        self.requires("grpc/1.54.3", transitive_libs=True, transitive_headers=True)
        self.requires("protobuf/3.21.12")
