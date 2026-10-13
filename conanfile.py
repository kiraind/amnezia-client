from conan import ConanFile

class AmneziaVPN(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "VirtualBuildEnv", "CMakeConfigDeps"

    options = {
        "macos_ne": [True, False]
    }
    default_options = {
        "macos_ne": False
    }

    def requirements(self):
        os = str(self.settings.os)

        has_ne = os == "iOS" or (os == "Macos" and self.options.macos_ne)
        has_service = os == "Windows" or os == "Linux" or (os == "Macos" and not has_ne)

        if has_service:
            if os == "Windows":
                self.requires("awg-windows/3.1.20260814")
                self.requires("win-split-tunnel/1.2.5.0")
                self.requires("wintun/0.14.1")
            else:
                self.requires("awg-go/3.1.20260814")

        if has_ne:
            self.requires("awg-apple/3.1.4")
            self.requires("hev-socks5-tunnel/2.15.0", options={"as_framework": True})
            self.requires("openvpnadapter/1.0.0")

        if os == "Android":
            self.requires("awg-android/3.1.20260814")

        self.requires("openssl/3.6.2")
        self.requires("zlib/1.3.2")
