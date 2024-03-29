newoption {
  trigger = "with-tests",
  description = "Build tests"
}

workspace "sln-chromium_base"
  objdir "builddir/obj"
  targetdir "builddir"
  libdirs { "builddir" }
  -- defines { "WINVER=0x0501", "_WIN32_WINNT=0x0501", "_WIN32_WINDOWS=0x0501" }

  configurations { "Debug", "Release" }

  filter "configurations:Debug"
     defines { "DEBUG" }
     symbols "On"

  filter "configurations:Release"
     defines { "NDEBUG" }
     optimize "On"

  project "chromium_base"
    kind "StaticLib"
    language "C++"
    files { "src/base/**.cc" }
    removefiles {
      "src/base/third_party/dmg_fp/dtoa.cc",
      "src/base/**_win.cc",
      "src/base/win/**.cc",
      "src/base/**_posix.cc",
      "src/base/posix/**.cc",
      "src/base/**_linux.cc",
    }

    includedirs { "src" }
    defines { "UNICODE" }

    filter "action:vs*"
      files {
        "src/base/**_win.cc",
        "src/base/win/**.cc"
      }
      buildoptions { "/std:c++14" }

    filter { "system:windows", "action:gmake" }
      files {
        "src/base/**_win.cc",
        "src/base/win/**.cc"
      }
      removefiles {
        "src/base/message_loop/**",
        "src/base/threading/framework_thread.cc",
        "src/base/threading/thread_manager.cc",
        "src/base/timer/timer.cc",
        "src/base/win/object_watcher.cc",
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
        "src/base/threading/framework_thread.cc",
        "src/base/threading/thread_manager.cc",
      }
      buildoptions { "-std=c++14", "-fno-rtti", "-Wno-user-defined-warnings" }


if _OPTIONS["with-tests"] then
  project "chromium_base_unittest"
    kind "ConsoleApp"
    language "C++"

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
end
