let g:neomake_c_enabled_makers = ['clang']
let g:neomake_cpp_enabled_makers = ['clang']

let g:neomake_clang_args = ['-fsyntax-only', '-Wall', '-Wextra', '-I./src', '-std=c++14']
