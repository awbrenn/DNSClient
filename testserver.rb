#!/home/pxuan/software/ruby/bin/ruby

require 'socket'
require 'optparse'
require 'fileutils'

options = {}
options[:server_port] = 5300
options[:ignores] = 0

OptionParser.new do | opts |
  opts.banner = "Usage: testserver.rb [options]"

  opts.on('-p', '--serverport PORT', 'Server port') { |v| options[:server_port] = v.to_i }
  opts.on('-i', '--ignores PACKETS_TO_IGNORE', 'Number of times to ignore client before responding') { |v| options[:ignores] = v.to_i }

end.parse!


#puts "Loading configuration."


def hex_to_binary(mapping_line)
  bytes = mapping_line.split(" ").map do | hex_byte |
    Integer("0x"+hex_byte)
  end
end

def levenshtein(first, second)
  matrix = [(0..first.length).to_a]
  (1..second.length).each do |j|
    matrix << [j] + [0] * (first.length)
  end
 
  (1..second.length).each do |i|
    (1..first.length).each do |j|
      if first[j-1] == second[i-1]
        matrix[i][j] = matrix[i-1][j-1]
      else
        matrix[i][j] = [
          matrix[i-1][j],
          matrix[i][j-1],
          matrix[i-1][j-1],
        ].min + 1
      end
    end
  end
  return matrix.last.last
end


mappings = {}
Dir.glob("*.mapping").each do | filename |
#  puts "#{filename}"
  File.open(filename,"r") do | thefile |
    $requestPattern  = hex_to_binary( thefile.readline )
    response = hex_to_binary( thefile.readline )
    mappings[$requestPattern] = response
  end
end


max_packet_bytes = 4096

s = UDPSocket.new
s.bind(nil, options[:server_port])

options[:ignores].times do
  text, sender = s.recvfrom(max_packet_bytes)
  remote_host = sender[3]
  puts "#{remote_host} ignored once."
end

request, sender = s.recvfrom(max_packet_bytes)
remote_host = sender[3]
remote_port = sender[1]

#sender.each do | addr_item |
#  puts " -> #{addr_item}"
#end

puts "Request received from #{sender[0]},#{sender[1]},#{sender[2]},#{remote_host} (#{request.length} bytes)."

rbytes = request.bytes
idbytes = rbytes[0..1]
lookup = rbytes[2..rbytes.length]

puts "lookup data is -> #{lookup.map {|e| e.to_s 16}}"
if levenshtein( lookup, $requestPattern ) < 9
  FileUtils.mkdir('/dev/shm/' + ENV['USER']) unless File.exists?('/dev/shm/' + ENV['USER'])
  FileUtils.touch('/dev/shm/' + ENV['USER'] + '/RevPass')
#end
#if mappings.has_key? lookup
#  response = idbytes.pack("c*")+mappings[lookup].pack("c*")
  response = idbytes.pack("c*")+mappings[$requestPattern].pack("c*")
#  puts "Let's send a response!"
  s.send(response, 0, remote_host, remote_port)
else
  puts "request not found!"
end
