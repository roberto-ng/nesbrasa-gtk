teste_jmp_bug = executable('jmp_bug', 'jmp_bug.cpp',
                     include_directories: [inc, inc_mapeadores],
                     link_with: nesbrasa_lib)

test('Testar bug do JMP indireto', teste_jmp_bug, args: [])
