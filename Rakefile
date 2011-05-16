desc "Clean up after LMFAO"
task :clean do
  sh 'git clean -fX ext/'
end

desc "Compile LMFAO"
task :compile do
  Dir::chdir('ext') { sh 'ruby extconf.rb && make' }
end

desc "Run LMFAO"
task :default => [:clean, :compile] do
  exec 'ruby', 'lmfao.rb'
end