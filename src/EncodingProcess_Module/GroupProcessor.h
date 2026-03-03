#pragma once
#include<string>
#include<cstddef>

// 用來判斷是否是字元中的特殊符號用的介面
class ISymbolJudgment{
public:
  virtual ~ISymbolJudgment() = default;
  virtual bool judgment(const std::string& content) = 0;
};

// 介面 用來給處理 utf 群組的類別繼承
class myRtfGroupProcessor{
public:
  virtual ~myRtfGroupProcessor() = default;
  virtual void processGroups(std::string& content) = 0;
};


// 判斷該群組中是否都是 utf 特殊符號
class utfSymbolJudgment : public ISymbolJudgment{
public:
   bool judgment(const std::string& content) override;  
private:  
  unsigned int utf8ToCodepoint(const std::string& s, size_t& i);
  bool isAllowedSymbol(unsigned int cp);
};

// utf8 用的群組處理類別
class utf8GroupProcessor : public myRtfGroupProcessor{
public:
  void processGroups(std::string& content) override;
private:
  void cleanTargetGroup(std::string& content,const std::string& target);
  void handleSpecialSymbolGroups(std::string& content);
};
