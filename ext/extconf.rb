# coding: utf-8
require 'mkmf'

def must_have(type, ident)
  abort "[ERROR]: missing `#{ident}`. Aborting." unless send("have_#{type}", ident)
end

must_have :header, 'ruby.h'
must_have :func, 'rb_thread_blocking_region'
must_have :library, 'pthread'

$CFLAGS << '-O0 -ggdb -Wall'

create_makefile 'lmfao_ext'