/*
   MIT License

   Copyright (c) 2018 santhoshachar08@gmail.com

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#ifndef ENROLL_HPP
#define ENROLL_HPP

#include<string>
#include<vector>

#include "NotCopyable.hpp"

class Enroll : public NotCopyable
{
  private:
    std::string &FacesFile;
    std::string &LableFile;
    std::string SessionId;
    bool ReadyToEnroll;
    std::vector<std::string> ImageFilesVec;
    std::vector<int> LablesVec;
    bool CheckFileExists(std::string &file);
    std::vector<std::string> ReadFile(std::string &file);
  public:
    Enroll(std::string &facesFile, std::string &lableFile);
    ~Enroll();
    bool Run(std::string &sessId);
};

#endif // ENROLL_HPP