#!/usr/bin/env ruby

unless ARGV.size == 2
  puts "Pass width and height"
  exit(-1)
end

width = Integer(ARGV[0])
height = Integer(ARGV[1])

image = height.times.map do
  width.times.map { Random.rand(256) }.join(' ')
end

puts "P2"
puts "#{width} #{height}"
puts 255
puts image
