#pragma once
#include<cstddef>
#include<atomic>
#include<filesystem>
#include<vector>

// forward delecration
namespace Cli{
  struct ParseResult;
}

// 用來接收進度資訊並決定如何呈現的類別
class ProgressObserver{
  size_t total_{};
  std::atomic_size_t done_{0}; 
public:
   void Start(size_t num);
   void onUnitDone();
   void Finish();
private:
   void display() const;
};

// 多執行緒任務的總管
class RTFDirectoryRunner{
public:
  void run(const std::filesystem::path& dirPath,ProgressObserver& ProOB,const Cli::ParseResult& pr);
private:
  std::vector<std::filesystem::path> collectRtfFiles(const std::filesystem::path& dirPath);
  size_t DecideThreadNum(size_t resultCount);
};

