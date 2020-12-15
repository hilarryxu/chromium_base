workspace "sln-chromium_base"
  objdir "builddir/obj"

  configurations { "Debug", "Release" }

  configuration "Debug"
     defines { "DEBUG" }
     symbols "On"

  configuration "Release"
     defines { "NDEBUG" }
     optimize "On"

  project "chromium_base"
    kind "StaticLib"
    language "C++"
    targetdir "builddir"
    files { "src/base/**.cc" }
    removefiles { "src/base/third_party/dmg_fp/dtoa.cc" }
    removefiles {
      "src/base/**_win.cc",
      "src/base/win/**.cc",
      "src/base/**_posix.cc",
      "src/base/posix/**.cc",
      "src/base/**_linux.cc"
    }

    includedirs { "src" }
    libdirs { "builddir" }
    defines { "UNICODE" }

    filter "action:vs*"
      files {
        "src/base/**_win.cc",
        "src/base/win/**.cc"
      }
      buildoptions { "/std:c++14" }

    filter { "system:windows", "toolset:gcc" }
      files {
        "src/base/**_win.cc",
        "src/base/win/**.cc"
      }
      defines { "MINGW_HAS_SECURE_API", "_POSIX_C_SOURCE" }
      buildoptions { "-std=c++14", "-fno-rtti" }

    filter "system:linux"
      files {
        "src/base/**_posix.cc",
        "src/base/posix/**.cc",
        "src/base/**_linux.cc"
      }
      removefiles {
        "src/base/message_loop/**",
        "src/base/threading/thread.cc",
        "src/base/threading/framework_thread.cc",
        "src/base/threading/thread_manager.cc",
      }
      buildoptions { "-std=c++14", "-fno-rtti" }

    filter "system:macosx"
      files {
        "src/base/**_posix.cc",
        "src/base/posix/**.cc"
      }
      removefiles {
        "src/base/message_loop/**",
        "src/base/threading/thread.cc",
        "src/base/threading/framework_thread.cc",
        "src/base/threading/thread_manager.cc",
      }
      buildoptions { "-std=c++14", "-fno-rtti", "-Wno-user-defined-warnings" }

  project "chromium_base_unittest"
    kind "ConsoleApp"
    language "C++"
    targetdir "builddir"

    files { "tests/**.cc" }
    includedirs { "src", "tests", "third_party" }
    defines { "UNICODE" }

    filter "system:linux"
      buildoptions { "-std=c++14", "-fno-rtti" }
      links { "chromium_base", "pthread" }

    filter "system:windows"
      links { "chromium_base", "winmm", "shlwapi", "Ole32", "Dbghelp" }

    filter "system:macosx"
      buildoptions { "-std=c++14", "-fno-rtti" }
      links { "chromium_base", "pthread" }
