# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

require 'optparse'
require "ostruct"
require "webrick/server"

module Milter
  class Client
    class CommandLine
      attr_reader :options, :option_parser
      def initialize
        setup_options
        setup_option_parser
      end

      def run(argv=nil)
        begin
          @option_parser.parse!(argv || ARGV)
        rescue
          puts $!.message
          puts @option_parser
          exit(false)
        end
        client = Milter::Client.new
        client.status_on_error = @options.status_on_error
        client.connection_spec = @options.connection_spec
        yield(client, options) if block_given?
        daemonize if @options.run_as_daemon
        client.main
      end

      private
      def setup_options
        @options = OpenStruct.new
        @options.connection_spec = "inet:20025"
	@options.status_on_error = "accept"
        @options.run_as_daemon = false
      end

      def setup_option_parser
        @option_parser = OptionParser.new(banner)
        setup_common_options
      end

      def banner
        "Usage: %s [options]" % File.basename($0, '.*')
      end

      def setup_common_options
        setup_basic_options
        setup_milter_options
        setup_logger_options
      end

      def setup_basic_options
        @option_parser.separator ""
        @option_parser.separator "Basic options"

        @option_parser.on("--help", "Show this message.") do
          puts @option_parser
          exit(true)
        end

        @option_parser.on("--version", "Show version.") do
          puts VERSION
          exit(true)
        end
      end

      def setup_milter_options
        @option_parser.separator ""
        @option_parser.separator "milter options"

        @option_parser.on("-s", "--connection-spec=SPEC",
                "Specify connection spec as [SPEC].",
                "(#{@options.connection_spec})") do |spec|
          @options.connection_spec = spec
        end

        @option_parser.on("--[no-]daemon",
                "Run as a daemon process.",
                "(#{@options.run_as_daemon})") do |run_as_daemon|
          @options.run_as_daemon = run_as_daemon
        end

        statuses = ["accept", "reject", "temporary_failure"]
        @option_parser.on("--status-on-error=STATUS",
                statuses,
                "Specify status on error.",
                "(#{@options.status_on_error})") do |status|
          @options.status_on_error = status
        end

        @option_parser.on("--database-path=PATH",
                "Specify database path to store messages as [PATH].",
                "(#{@options.database_path})") do |path|
          @options.database_path = path
        end
      end

      def setup_logger_options
        @option_parser.separator ""
        @option_parser.separator "Logging @options"

        level_names = Milter::LogLevelFlags.values.collect {|value| value.nick}
        level_names << "all"
        @option_parser.on("--log-level=LEVEL",
                "Specify log level as [LEVEL].",
                "Select from [%s]." % level_names.join(', '),
                "(#{ENV['MILTER_LOG_LEVEL'] || 'none'})") do |level|
          if level.empty?
            ENV["MILTER_LOG_LEVEL"] = nil
          else
            ENV["MILTER_LOG_LEVEL"] = level
          end
        end

        @option_parser.on("--verbose",
                "Show messages verbosely.",
                "Alias of --log-level=all.") do
          ENV["MILTER_LOG_LEVEL"] = "all"
        end
      end

      def daemonize
        WEBrick::Daemon.start
      end
    end
  end
end
