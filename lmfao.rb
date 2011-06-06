# coding: utf-8
require_relative 'ext/lmfao_ext'
require 'minitest/unit'
include MiniTest::Assertions

begin
  puts "Welcome to LMFAO self-test!"
  puts
  print "Give me what to LMFAO: "
  called = false
  result = LMFAO::call(input = gets.chomp) do |string|
    called = true
    assert(Thread.current != Thread.main, "Expected callback not to be in main thread.")
    string.upcase
  end
  puts "LMFAO result received."

  assert_equal(input.upcase, result)
  assert called, "Expected callback to have been called."

  puts "LMFAO result: #{result}. Success!"
rescue MiniTest::Assertion => e
  puts "FAIL: #{e.message}"
end
