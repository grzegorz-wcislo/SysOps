#!/usr/bin/env ruby

unless ARGV.size == 1
  puts "Pass size"
  exit(-1)
end

size = Integer(ARGV[0])

filter = size.times.map do
  size.times.map do
    Random.rand(1.0)
  end
end

sum = filter.map(&:sum).sum

filter.map! do |row|
  row.map { |el| el / sum }.join(' ')
end

puts size
puts filter
