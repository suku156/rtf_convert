#pragma once

// forward declaration
namespace Cli{
  struct  ParseResult; 
}

namespace App{
  enum class AppExitCode{
    Success,
    Fail,
    CliError,
    RunTimeError
  };

  class ConversionEngine{
  public:
  AppExitCode run(const Cli::ParseResult& pr);
  private:
  }; 
}
