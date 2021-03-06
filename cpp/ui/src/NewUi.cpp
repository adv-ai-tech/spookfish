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



#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>

#include <sys/stat.h>

// system() cmd related.
#include <stdlib.h>
#include <signal.h>

#include <Wt/WMessageBox.h>
#include <Wt/WAnchor.h>
#include <Wt/WTable.h>
#include <Wt/WLength.h>

#include "NewUi.hpp"
#include "TcpClient.hpp"
#include "TcpServer.hpp"
#include "JsonFileParser.hpp"

// GetImageFiles related includes.
#include <sys/types.h>
#include <dirent.h>

bool NewUiApplication::SetupDefaultDirectories()
{
  std::string cmd("mkdir -p /tmp/images/" + sessionId());
  ExecuteCommand(cmd);
  return true;
}

int NewUiApplication::ExecuteCommand(std::string &cmd)
{
  std::cout << "Executing CMD: " << cmd.c_str() << "\n";
  int ret = system(cmd.c_str());
  if (WIFSIGNALED(ret)
      && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) {
    std::cerr << "ERROR: Failed to execute command " << cmd.c_str() << "\n";
  }
  return ret;
}

void NewUiApplication::SetupRefreshTimer()
{
  Timer = root()->addChild(std::make_unique<Wt::WTimer>());
  Timer->setInterval(std::chrono::seconds(10));
  Timer->timeout().connect(this, &NewUiApplication::TimeOutReached);
  Timer->start();

  // TODO: This mechanism is a work around for the Demo purpose.
  // Need to remove in the deployment scenario.
  std::string zoomPath("/tmp/images/" + sessionId() + "/zoom_shot");
  std::string cmd("mkdir -p " + zoomPath);
  ExecuteCommand(cmd);
  std::string onePath("/tmp/images/" + sessionId() + "/1");
  cmd = "mkdir -p " + onePath;
  ExecuteCommand(cmd);
}

bool NewUiApplication::CheckFileExists(std::string &file)
{
  struct stat buffer;
  bool val(false);
  if ((stat(file.c_str(), &buffer) == 0)) {
    val = true;
    std::cout << "File " << file.c_str() << " Found.\n";
  }
  else {
    std::cout << "File " << file.c_str() << " Not Found.\n";
  }
  return val;
}

bool NewUiApplication::StopTimer(std::string &file)
{
  bool val(false);
  std::cout << "StopTimer : " << file.c_str() << "\n";
  if (CheckFileExists(file)) {
    std::cout << "Done with video analysis stopping the timer.\n";
    Timer->stop();
    val = true;
  }
  return val;
}

NewUiApplication::TImageAnchorLinkMap NewUiApplication::GetImageFiles(std::string &file)
{
  std::cout << "Path : " << file.c_str() << "\n";
  std::string line("");
  NewUiApplication::TImageAnchorLinkMap imgAnchorMap;
  std::ifstream infile(file);
  while (std::getline(infile, line)) {
    std::istringstream iss(line);
    std::vector<std::string> vec((std::istream_iterator<std::string>(iss)),
      std::istream_iterator<std::string>());
    std::cout << " vec[0] = " << vec[0] << " vec[1] = " << vec[1] << "\n";
    if (!vec[0].empty() && !vec[1].empty()) {
     /* std::string key("." + vec[0].substr(4));
      std::string val("." + vec[0].substr(4));
      imgAnchorMap[key] = val;*/
      imgAnchorMap[vec[0]] = vec[1];
    }
  }
  return imgAnchorMap;
}

void NewUiApplication::TimeOutReached()
{
  std::cout << "Timeout occured refreshing\n";
  std::string imgAnchFile("");
  std::string basePath("/tmp/images/");
  std::string sessId(sessionId());
  //std::string sessId("xcJHt7IOEdHWyOck");
  if (IsClusterEnabled) {
    std::string imgDir("/cluster/");
    imgAnchFile = basePath + sessId + "/clusterImagesAnchor.txt";
  }
  else {
    std::string imgDir("/zoom_shot/");
    imgAnchFile = basePath + sessId + "/zoomImagesAnchor.txt";
  }

  root()->removeWidget(MainImageGallaryDiv);
  MainImageGallaryDiv = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
  MainImageGallaryDiv->setId("gallary");
  NewUiApplication::TImageAnchorLinkMap imgAncMap(GetImageFiles(imgAnchFile));
  if (!IsClusterResized && IsClusterEnabled) {
    IsClusterResized = true;
    for(int i = 0; i < imgAncMap.size(); i++) {
      PersonNameVector.push_back("Unknown");
    }
    PersonNameVector.resize(imgAncMap.size());
  }
  SetupImageGallary(MainImageGallaryDiv, imgAncMap);

  root()->removeWidget(FooterDiv);
  SetupFooter();
  root()->refresh();
  std::string doneFile(basePath + sessId + "/cluster/Done.txt");
  //std::string doneFile(basePath + sessionId() + "/cluster/Done.txt");
  StopTimer(doneFile);
}

void NewUiApplication::SetupTheme()
{
  //setCssTheme("bootstrap3");
  useStyleSheet(Wt::WLink("css/styleSheet.css")); // TODO: Configurable.
  useStyleSheet(Wt::WLink("resources/themes/bootstrap/3/bootstrap.min.css"));
  //useStyleSheet(Wt::WLink("resources/main.css")); // TODO: Configurable.
}

void NewUiApplication::SetupHeader()
{
  Wt::WContainerWidget *HeaderDiv = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
  HeaderDiv->setId("header");
  Wt::WContainerWidget *HeaderDivTextDiv = HeaderDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  HeaderDivTextDiv->setId("h3");
  Wt::WText *HeaderText = HeaderDivTextDiv->addWidget(std::make_unique<Wt::WText>(Wt::WString::fromUTF8("spookfish")));
}

void NewUiApplication::SetupVideoSearchBar(Wt::WContainerWidget *mainLeft)
{
  Wt::WContainerWidget *RowDiv = mainLeft->addWidget(std::make_unique<Wt::WContainerWidget>());
  RowDiv->setStyleClass("row");

  Wt::WContainerWidget *ColumnLeftDiv = RowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  ColumnLeftDiv->setStyleClass("col-md-3  col-xs-1 col-sm-1");

  Wt::WContainerWidget *ColumnMiddleDiv = RowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  ColumnMiddleDiv->setStyleClass("col-md-6  col-xs-3 col-sm-3");

  Wt::WContainerWidget *SearchDiv = ColumnMiddleDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  SearchDiv->setStyleClass("input-group");
  Wt::WContainerWidget *SearchDivLineEditDiv = SearchDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  SearchLineEdit = SearchDivLineEditDiv->addWidget(std::make_unique<Wt::WLineEdit>(Wt::WString::fromUTF8("Give me Youtube URL")));
  //SearchLineEdit = SearchDivLineEditDiv->addWidget(std::make_unique<Wt::WLineEdit>(Wt::WString::fromUTF8("https://www.youtube.com/watch?v=giYeaKsXnsI")));
  SearchLineEdit->setStyleClass("form-control");
  SearchLineEdit->setFocus();
  Wt::WContainerWidget *ButtonDiv = SearchDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  ButtonDiv->setStyleClass("input-group-btn");
  PlayButton = ButtonDiv->addWidget(std::make_unique<Wt::WPushButton>("Play"));
  PlayButton->setStyleClass(Wt::WString::fromUTF8("btn btn-success with-label"));

  Wt::WContainerWidget *ColumnRightDiv= RowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  ColumnRightDiv->setStyleClass("col-md-3  col-xs-1 col-sm-1");

  SearchLineEdit->enterPressed().connect
    (std::bind(&NewUiApplication::OnPlayButtonPressed, this));

  PlayButton->clicked().connect(this, &NewUiApplication::OnPlayButtonPressed);
}

void NewUiApplication::SetupNavToolBar(Wt::WContainerWidget *navToolDiv)
{
  Wt::WContainerWidget *btnGrp = navToolDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  btnGrp->setStyleClass("btn-group btn-group-justified");

  Wt::WContainerWidget *allImagesBtnDiv= btnGrp->addWidget(std::make_unique<Wt::WContainerWidget>());
  allImagesBtnDiv->setStyleClass("btn-group");
  Wt::WPushButton *allImagesBtn = allImagesBtnDiv->addWidget(std::make_unique<Wt::WPushButton>("All-Images"));
  allImagesBtn->setStyleClass("btn btn-primary");

  Wt::WContainerWidget *clusterBtnDiv= btnGrp->addWidget(std::make_unique<Wt::WContainerWidget>());
  clusterBtnDiv->setStyleClass("btn-group");
  Wt::WPushButton *clusterBtn = clusterBtnDiv->addWidget(std::make_unique<Wt::WPushButton>("Cluster"));
  clusterBtn->setStyleClass("btn btn-info");

  Wt::WContainerWidget *enrollBtnDiv = btnGrp->addWidget(std::make_unique<Wt::WContainerWidget>());
  enrollBtnDiv->setStyleClass("btn-group");
  Wt::WPushButton *enrollBtn = enrollBtnDiv->addWidget(std::make_unique<Wt::WPushButton>("Enroll-New-Images"));
  enrollBtn->setStyleClass("btn btn-warning");

  Wt::WContainerWidget *statsBtnDiv= btnGrp->addWidget(std::make_unique<Wt::WContainerWidget>());
  statsBtnDiv->setStyleClass("btn-group");
  Wt::WPushButton *statsBtn = statsBtnDiv->addWidget(std::make_unique<Wt::WPushButton>("Stats"));
  statsBtn->setStyleClass("btn btn-primary");

  Wt::WContainerWidget *saveBtnDiv= btnGrp->addWidget(std::make_unique<Wt::WContainerWidget>());
  saveBtnDiv->setStyleClass("btn-group");
  Wt::WPushButton *saveBtn = saveBtnDiv->addWidget(std::make_unique<Wt::WPushButton>("Save"));
  saveBtn->setStyleClass("btn btn-success");

  Wt::WContainerWidget *delBtnDiv = btnGrp->addWidget(std::make_unique<Wt::WContainerWidget>());
  delBtnDiv->setStyleClass("btn-group");
  Wt::WPushButton *delBtn = delBtnDiv->addWidget(std::make_unique<Wt::WPushButton>("Delete-All-Images"));
  delBtn->setStyleClass("btn btn-danger");

  //MainNavToolDiv->hide();

  clusterBtn->clicked().connect(this, &NewUiApplication::OnClusterButtonPressed);
  allImagesBtn->clicked().connect(this, &NewUiApplication::OnAllImagesButtonPressed);
  saveBtn->clicked().connect(this, &NewUiApplication::OnSaveButtonPressed);
  enrollBtn->clicked().connect(this, &NewUiApplication::OnEnrollButtonPressed);
  delBtn->clicked().connect(this, &NewUiApplication::OnDeleteButtonPressed);
  statsBtn->clicked().connect(this, &NewUiApplication::OnStatsButtonPressed);
  //statsBtn->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/stats"));
}

void NewUiApplication::OnStatsButtonPressed()
{
  std::string file("/tmp/images/"+sessionId()+"/Stats.txt");
  std::map<std::string, std::string> statsMap;
  std::string line("");
  if (CheckFileExists(file)) {
    std::ifstream infile;
    std::string labelFile("/tmp/images/"+sessionId()+"/lable_name.txt");
    std::vector<std::string> actorNames;
    bool lableFileExists(CheckFileExists(labelFile));
    if (lableFileExists) {
      infile.open(labelFile);
      int idx(0);
      while (std::getline(infile, line)) {
        if (idx > 0) {
          int pos = line.find_first_of(";");
          std::string name = line.substr(0,pos);
          actorNames.push_back(name);
        }
        idx++;
      }
      infile.close();
    }

    infile.open(file);
    line.clear();
    std::cout << "########## Stats Begin ##############\n";
    int nameIndex = 0;
    while (std::getline(infile, line)) {
      std::cout << line.c_str() << "\n";
      int pos = line.find_first_of(":");
      std::string key(line.substr(0,pos));
      std::string counts(line.substr(pos+1));
      auto keyPair = std::make_pair(key,counts);
      statsMap.insert(keyPair);
      auto itr = statsMap.find("cluster_"+std::to_string(nameIndex));
      if ( actorNames.size() > 0 && itr != statsMap.end()) {
        std::cout << "Found old key = " << itr->first;
        counts = itr->second;
        statsMap.erase(key);
        std::cout << "Erasing Old key\n";
        auto mkpair = std::make_pair(actorNames[nameIndex], counts);
        statsMap.insert(mkpair);
        std::cout << "new key = " << key.c_str() << "\n";
        nameIndex++;
      }
    }
    std::cout << "########## Stats Ends  ##############\n";
    infile.close();
  }
  for(auto &v: statsMap) {
    std::cout << "KSS key = " << v.first << " val = " << v.second << "\n";
  }
  SetupStatsDisplay(statsMap);
  /*
  Wt::WLink anchorLink = Wt::WLink(file.c_str());
  anchorLink.setTarget(Wt::LinkTarget::NewWindow);
  std::unique_ptr<Wt::WAnchor> anchor = std::make_unique<Wt::WAnchor>(link);
  anchor->addNew<Wt::WImage>(Wt::WLink("https://www.emweb.be/css/emweb_small.png"));
*/
/*  Wt::WContainerWidget *statsDiv = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
  Wt::WAnchor *anchor = statsDiv->addWidget(std::make_unique<Wt::WAnchor>(anchorLink));*/
}

void NewUiApplication::OnDeleteButtonPressed()
{
  MainStatsDiv->hide();
  Wt::WMessageBox::show("Information", "Sorry, I only belive in constructive Actions! :)", Wt::StandardButton::Ok);
}

void NewUiApplication::OnEnrollButtonPressed()
{
  MainStatsDiv->hide();
  std::cout << "OnEnrollButtonPressed\n";
  std::string action("Enroll");
  std::string val("");
  SendVideoAnalysisRequest(action, val);
}

void NewUiApplication::OnSaveButtonPressed()
{
  MainStatsDiv->hide();
  std::cout << "OnSaveButtonPressed\n";
  //IsClusterEnabled = false;
  /*std::string srcPath("/tmp/images/" + sessionId());
  // TODO: make it configurable.
  std::string dstPath("/tmp/ImageStorage");
  std::string cmd("cp -r " + srcPath + " " + dstPath);
  ExecuteCommand(cmd);*/
  std::string cmd = "bash /opt/spookfish/scripts/save.sh";
  ExecuteCommand(cmd);
}

void NewUiApplication::OnAllImagesButtonPressed()
{
  MainStatsDiv->hide();
  std::cout << "OnAllImagesButtonPressed\n";
  IsClusterEnabled = false;
  TimeOutReached();
}

void NewUiApplication::OnClusterButtonPressed()
{
  MainStatsDiv->hide();
  std::string action("Cluster");
  std::string val("");
  //SendVideoAnalysisRequest(action, val);
  std::cout << "OnClusterButtonPressed \n";
  IsClusterEnabled = true;
  TimeOutReached();
}

void NewUiApplication::SetupVideoPlayer(Wt::WContainerWidget *mainLeft)
{
  MainVideoContainer = mainLeft->addWidget(std::make_unique<Wt::WContainerWidget>());
  Wt::WContainerWidget *rowDiv = MainVideoContainer->addWidget(std::make_unique<Wt::WContainerWidget>());
  rowDiv->setStyleClass("row");

  Wt::WContainerWidget *columnLeftDiv = rowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  columnLeftDiv->setStyleClass("col-md-3  col-xs-1 ");

  Wt::WContainerWidget *ColumnMiddleDiv = rowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  ColumnMiddleDiv->setStyleClass("col-md-6 col-xs-3");

  Wt::WContainerWidget *VideoPlayerDiv = ColumnMiddleDiv->addWidget(std::make_unique<Wt::WContainerWidget>());

  VideoPlayerDiv->setStyleClass("embed-responsive embed-responsive-4by3");
  VideoPlayer = VideoPlayerDiv->addWidget(std::make_unique<Wt::WVideo>());

  VideoPlaybackStatus = ColumnMiddleDiv->addWidget(std::make_unique<Wt::WText>("Playback Not started."));
  VideoPlaybackStatus->setStyleClass("videoStatus");
  Wt::WContainerWidget *columnRightDiv = rowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  columnRightDiv->setStyleClass("col-md-3  col-xs-1 ");

  std::string str("<p>Video playing</p>");
  VideoPlayer->playbackStarted().connect(std::bind(&NewUiApplication::SetVideoPlaybackStatus, this, str));
  str.clear();
  str = "<p>Video paused</p>";
  VideoPlayer->playbackPaused().connect(std::bind(&NewUiApplication::SetVideoPlaybackStatus, this, str));
  str.clear();
  str = "<p>Volume changed</p>";
  VideoPlayer->volumeChanged().connect(std::bind(&NewUiApplication::SetVideoPlaybackStatus, this, str));
  str.clear();
  str = "<p>Video Ended</p>";
  VideoPlayer->ended().connect(std::bind(&NewUiApplication::SetVideoPlaybackStatus, this, str));
  MainVideoContainer->hide();
}

void NewUiApplication::SetupImageGallary(Wt::WContainerWidget *mainRight, NewUiApplication::TImageAnchorLinkMap &imgAnchorLinkMap)
{
  Wt::WContainerWidget *gallaryDiv = mainRight->addWidget(std::make_unique<Wt::WContainerWidget>());
  gallaryDiv->setStyleClass("container-fluid");
  Wt::WContainerWidget *rowDiv = gallaryDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  int index(0);
  rowDiv->setStyleClass("row");
  std::string sessId(sessionId());
  //std::string sessId("xcJHt7IOEdHWyOck");
  std::string lableFile("/tmp/images/"+sessId+"/label_name.txt");
  bool lableExist(CheckFileExists(lableFile));
  std::vector<std::string> actorNamesVec;
  std::vector<std::string> enRollVec;
  if (lableExist) {
    std::cout << "Staring to create Face Directory\n";
    std::ifstream infile(lableFile);
    int lineNum(0);
    std::string line("");
    while (std::getline(infile, line)) {
      std::cout << "Line : " << line.c_str() << "\n";
      if (lineNum > 0) {
        int pos = line.find_first_of(";");
        std::string name(line.substr(0,pos));
        actorNamesVec.push_back(name);
        std::string clusterImgFile("/tmp/images/" + sessId + "/faces/" + name);
        std::cout << "clusterImgFile = " << clusterImgFile.c_str() <<"\n";
        if (!CheckFileExists(clusterImgFile)) {
          std::string cmd("mkdir -p " + clusterImgFile);
          std::cout << "Creating Directory : " << cmd.c_str() << "\n";
          ExecuteCommand(cmd);
        }
      }
      lineNum++;
    }
  }
  for (auto &link: imgAnchorLinkMap) {
    Wt::WContainerWidget *columnDiv = rowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
    columnDiv->setStyleClass("col col-md-2 col-xs-4");
    Wt::WContainerWidget *thumbnailDiv= columnDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
    thumbnailDiv->setStyleClass("thumbnail");
    std::string imgAnchorLink(link.second);
    Wt::WLink anchorLink = Wt::WLink(imgAnchorLink.c_str());
    anchorLink.setTarget(Wt::LinkTarget::NewWindow);
    Wt::WAnchor *anchor = thumbnailDiv->addWidget(std::make_unique<Wt::WAnchor>(anchorLink));
    std::string imgLink(link.first);
    anchor->addNew<Wt::WImage>(Wt::WLink(imgLink.c_str()));
    Wt::WContainerWidget *captionDiv = thumbnailDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
    if (IsClusterEnabled) {
        Wt::WContainerWidget *personDiv = columnDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
        auto edit = personDiv->addWidget(std::make_unique<Wt::WLineEdit>(Wt::WString::fromUTF8("Rename Me")));
        personDiv->setStyleClass("col col-md-8 col-xs-12");
        edit->setAttributeValue("role", Wt::WString("alert"));
        if (lableExist) {
          edit->setText(actorNamesVec[index].c_str());
          edit->setStyleClass("alert alert-success");
          std::string clusterImgFilePath("/tmp/images/" + sessId + "/faces/" + actorNamesVec[index] + "/");
          std::string cmd("cp -r " + imgAnchorLink + "\t" + clusterImgFilePath);
          ExecuteCommand(cmd);
          int pos = link.second.find_last_of("/");
          std::string clusterImageFileName(link.second.substr(pos+1));
          enRollVec.push_back(clusterImgFilePath + clusterImageFileName);
        }
        else {
          edit->setStyleClass("alert alert-danger");
        }
        edit->enterPressed().connect
          (std::bind(&NewUiApplication::OnPersonNameChanged, this, index));
        EditVector.push_back(edit);
    }
    else {
      Wt::WText *caption = captionDiv->addWidget(std::make_unique<Wt::WText>("Unknown"));
    }
    captionDiv->setId("caption");
    index++;
  }
  if (lableExist) {
    std::string line;
    for(int i = 0; i < enRollVec.size(); i++) {
      line += enRollVec[i] + "\n";
    }
    std::string file("/tmp/images/" + sessId + "/enroll_images.txt");
    std::cout << "Creating file : " << file.c_str() << "\n";
    std::ofstream out(file);
    out << line;
    out.close();
    IsClusterEnabled = true;
  }
  MainImageGallaryDiv->show();
  return;
}

void NewUiApplication::SetupFooter()
{
  FooterDiv = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
  FooterDiv->setId("footer");
  Wt::WContainerWidget *FooterDivTextDiv = FooterDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  Wt::WText *Text = FooterDivTextDiv->addWidget(std::make_unique<Wt::WText>(Wt::WString::fromUTF8("© Spookfish. All Rights Reserved.")));
}

void NewUiApplication::SetupProgressBar(Wt::WContainerWidget *div)
{
  Wt::WContainerWidget *mainProgressDiv = div->addWidget(std::make_unique<Wt::WContainerWidget>());
  mainProgressDiv->setId("customProgressBar");

  Wt::WContainerWidget *row = mainProgressDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  row->setStyleClass("row");

  Wt::WContainerWidget *colDiv1 = row->addWidget(std::make_unique<Wt::WContainerWidget>());
  colDiv1->setStyleClass("col-md-3  col-xs-1 ");

  Wt::WContainerWidget *colDiv2 = row->addWidget(std::make_unique<Wt::WContainerWidget>());
  colDiv2->setStyleClass("col-md-6 col-xs-3");
  Wt::WContainerWidget *progressDiv = colDiv2->addWidget(std::make_unique<Wt::WContainerWidget>());
  //Wt::WContainerWidget *progressDiv = div->addWidget(std::make_unique<Wt::WContainerWidget>());
  progressDiv->setStyleClass("progress");

  Wt::WContainerWidget *progressBarDiv = progressDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  //progressBarDiv->setStyleClass("progress-bar progress-bar-striped active");
  progressBarDiv->setStyleClass("progress-bar progress-bar-success progress-bar-striped active");
  //progressBarDiv->setStyleClass("progress-bar");
  //progressBarDiv->setAttributeValue("class",Wt::WString("progress-bar"));
/*  progressBarDiv->setAttributeValue("role",Wt::WString("progressbar"));
  progressBarDiv->setAttributeValue("aria-valuenow", Wt::WString("100"));
  progressBarDiv->setAttributeValue("aria-valuemin", Wt::WString("0"));
  progressBarDiv->setAttributeValue("aria-valuemax", Wt::WString("100"));*/
  progressBarDiv->setAttributeValue("style", Wt::WString("min-width: 2em; width: 100%;"));
   //aria-valuenow="45" aria-valuemin="0" aria-valuemax="100" style="width: 45%">

  Wt::WContainerWidget *currentProgress = progressBarDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  currentProgress->setStyleClass("sr-only");

  Wt::WText *status = currentProgress->addWidget(std::make_unique<Wt::WText>());
  status->setText("45\% complete");

  Wt::WContainerWidget *colDiv3 = row->addWidget(std::make_unique<Wt::WContainerWidget>());
  colDiv3->setStyleClass("col-md-3  col-xs-1 ");
}

#if 0
void NewUiApplication::CreateServer(Wt::WContainerWidget *ptr)
{
  int port(5678);
  TcpServer server(port);
  std::cout << "Server Listening on port: " << port << "\n";
  while (true) {
    server.Accept();
    std::string jsonRequest = server.Read();
    //std::string replay(handler.ProcessRequest(jsonRequest));
    std::string replay("300 OK");
    // TODO: map the show() to the session Id.
    //MainImageGallaryDiv->show();
    ptr->show();
    std::cout << " Beyond SHOW\n";
    server.Send(replay);
  }
  server.Close();
}
#endif

void NewUiApplication::OnPersonNameChanged(int index)
{
  std::string actorName(EditVector[index]->text().toUTF8());
  if (PersonNameVector.empty()) {
    for(int i=0; i< 9; i++) {
      PersonNameVector.push_back("Unknown");
    }
  }
  std::cout << "Actor Name given: " << actorName.c_str() << "\n";
  if (actorName.compare("Rename Me") == 0) {
    std::cout << "Nothing changed return\n";
    return;
  }
  /*
  for(int i = 0; i < PersonNameVector.size(); i++) {
    std::cout << "index = " << i << " name = " << PersonNameVector[i].c_str() << "\n";
    EditVector[i]->setAttributeValue("role", Wt::WString("alert"));
    if (PersonNameVector[i].compare("Unknown") == 0) {
      EditVector[i]->setStyleClass("alert alert-danger");
    }
    else if (EditVector[index]->text().toUTF8().compare("Rename Me") != 0) {
      PersonNameVector[index] = actorName;
      EditVector[i]->setStyleClass("alert alert-success");
    }
  }
  */
  bool nameChanged(false);
  if (PersonNameVector[index].compare("Unknown") || PersonNameVector[index].compare("Rename Me")) {
    PersonNameVector[index] = actorName;
    EditVector[index]->setStyleClass("alert alert-success");
    nameChanged = true;
    std::cout << "Done Rename to : " << actorName.c_str() << "\n";
  }

  bool allowLabelFile(true);
  for(auto &v: PersonNameVector) {
    if ( (v.compare("Unknown") == 0) || (v.compare("Rename Me") == 0)) {
      allowLabelFile = false;
      break;
    }
  }

  if (allowLabelFile && nameChanged) {
    std::string line("Unknown;-1\n");
    for(int i = 0; i < PersonNameVector.size(); i++) {
      line += PersonNameVector[i]+ ";" + std::to_string(i) + "\n";
    }
    // TODO: Remove after testing.
    std::string sessId(sessionId());
    //std::string sessId("xcJHt7IOEdHWyOck");
    std::string file("/tmp/images/" + sessId + "/label_name.txt");
    std::cout << "Writing the File: " << file.c_str() << "\n";
    IsClusterEnabled = true;
    std::ofstream out(file);
    out << line;
    out.close();
    TimeOutReached();
    Wt::WMessageBox::show("Information",
        "Congragulations! you have new faces to Enroll for Face Recognition." , Wt::StandardButton::Ok);
  }
  else {
    Wt::WMessageBox::show("Information", "Please Rename remaining Highlighted Person's name with Red background." , Wt::StandardButton::Ok);
  }
}

// Event and button click handlers.
void NewUiApplication::OnPlayButtonPressed()
{
  auto url(SearchLineEdit->text().toUTF8());
  // little sanity check on the URL.
  //std::string jsonRequst("{\"URL\":\"youtube.com\"}");
  std::string sessId(sessionId());
  std::cout << "OnPlayButtonPressed\n";
  if (url.find("https://www.youtube.com") != std::string::npos
      || url.find("http://www.youtube.com") != std::string::npos)  {
    std::string cmd("python ./ui/scripts/youtube.py -u "); // TODO: Change the path.
    std::string file("/tmp/images/"+ sessId + "/playablevidlink.txt");
    std::string redirect(" > " + file );
    cmd = cmd + url + redirect;
    ExecuteCommand(cmd);
    std::ifstream infile(file);
    std::string line;
    while (std::getline(infile, line)) {
      if (!line.empty()) {
        VideoPlayer->show();
        VideoPlayer->clearSources();
        VideoPlayer->addSource(Wt::WLink(line));
        std::string action("Play");
        SendVideoAnalysisRequest(action, line);
        break;
      }
    }
  }
  else {
    VideoPlayer->hide();
    Wt::WMessageBox::show("Information", "Please give me youtube URL.", Wt::StandardButton::Ok);
  }
  MainVideoContainer->show();
  MainNavToolDiv->show();
}

bool NewUiApplication::SendVideoAnalysisRequest(std::string &action, std::string &val)
{
  std::string serverIp("localhost");
  int port(5678);
  std::string jsonFile("./ui/data/Request_format.json");
  JsonFileParser parser(jsonFile);

  std::string youTubeUrlKey("Youtube_URL");
  std::string sessIdKey("Session_Id");
  std::string plyUriKey("Rtp_Stream_URL");
  std::string actionKey("Action");

  std::string youTubeUrlVal(SearchLineEdit->text().toUTF8());
  // Each client connection will have an unique sessionID.
  // same can be used if the user gives multiple youtube links
  // to analyse. And controll the rest of the video processing.
  std::string sessIdVal(sessionId());

  //std::string sessId("xcJHt7IOEdHWyOck");
  //parser.SetString(sessIdKey, sessId);
  parser.SetString(sessIdKey, sessIdVal);
  parser.SetString(actionKey, action);
  if (action.compare("Play") == 0) {
    parser.SetString(plyUriKey , val);
  }
  else {
    std::string tempStr("InvalidUrl");
    parser.SetString(plyUriKey , tempStr);
    //parser.SetNull(plyUriKey);
  }
  parser.SetString(youTubeUrlKey, youTubeUrlVal);

  std::string jsonRequst(parser.GetStrigifiedJson());
  //std::cout << " Stringified obj: " << parser.GetStrigifiedJson().c_str();
  TcpClient client(serverIp, port);
  std::string result(client.Connect(jsonRequst));

  if (result.compare("200 OK") == 0) {
    std::cout << "Success: " << result.c_str() << "\n";
  }
  else if (result.compare("300 OK") == 0) {
    std::cout << "Analysis running: " << result.c_str() << "\n";
  }
  else {
    std::cout << "Failed: " << result.c_str() << "\n";
  }
  return true;
}

void NewUiApplication:: SetupStatsDisplay(std::map<std::string, std::string> &statsMap)
{

 // root()->removeWidget(MainStatsDiv);
  root()->removeWidget(FooterDiv);

  auto textDiv = MainStatsDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  Wt::WContainerWidget *rowDiv = textDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  rowDiv->setStyleClass("row");

  Wt::WContainerWidget *columnLeftDiv = rowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  columnLeftDiv->setStyleClass("col-md-3  col-xs-1 col-sm-1");

  Wt::WContainerWidget *columnCenterDiv = rowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  columnCenterDiv->setStyleClass("col-md-6  col-xs-1 col-sm-1");
  //auto text = textDiv->addWidget(std::make_unique<Wt::WText>("SIMPLE STATS HERE"));
  auto table = columnCenterDiv->addWidget(std::make_unique<Wt::WTable>());
  table->setHeaderCount(1);
  table->addStyleClass("table form-inline table-bordered table-striped");
  table->setWidth(Wt::WLength("100%"));
  table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("#"));
  table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Stats Description"));
  table->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Count"));
  int i = 1;
  for(auto itr = statsMap.begin(); itr != statsMap.end(); itr++, i++) {
    table->elementAt(i, 0)->addWidget(std::make_unique<Wt::WText>(std::to_string(i).c_str()));
    table->elementAt(i, 1)->addWidget(std::make_unique<Wt::WText>(itr->first.c_str()));
    table->elementAt(i, 2)->addWidget(std::make_unique<Wt::WText>(itr->second.c_str()));
  }
  Wt::WContainerWidget *columnRightDiv = rowDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
  columnRightDiv->setStyleClass("col-md-3  col-xs-1 col-sm-1");
  std::cout << "Ready to display\n";
  MainStatsDiv->show();
  SetupFooter();
  std::cout << "Done Footer creation to display\n";
  root()->refresh();
  std::cout << "Done With the table exiting\n";
}

void NewUiApplication::SetVideoPlaybackStatus(const std::string str)
{
  VideoPlaybackStatus->setText(str.c_str());
  VideoPlaybackStatus->setStyleClass("videoStatus");
}

NewUiApplication::NewUiApplication(const Wt::WEnvironment& env)
  : WApplication(env),
  IsClusterEnabled(false),
  IsClusterResized(false)
{
  SetupDefaultDirectories();
  setTitle("Spookfish UI"); // application title
  SetupTheme();
  SetupHeader();
  SetupRefreshTimer();
  MainVideoDiv = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
  MainVideoDiv->setId("main_left");
  SetupVideoSearchBar(MainVideoDiv);
  SetupVideoPlayer(MainVideoDiv);

  MainNavToolDiv = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
  MainNavToolDiv->setId("gallaryNavBar");
  SetupNavToolBar(MainNavToolDiv);

  Wt::WContainerWidget *ProgressBar= root()->addWidget(std::make_unique<Wt::WContainerWidget>());
  ProgressBar->setId("customProgressBar");
  SetupProgressBar(ProgressBar);

  MainImageGallaryDiv = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
  MainImageGallaryDiv->setId("gallary");
  MainImageGallaryDiv->hide();

  MainStatsDiv = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
  MainStatsDiv->hide();
  SetupFooter();
  std::vector<std::string> tmp;
  ClusterActorMap[sessionId()] = tmp;

  //std::thread responseThread(&NewUiApplication::CreateServer, this, MainImageGallaryDiv);
  //responseThread.detach();
}

NewUiApplication::~NewUiApplication()
{
  // Empty;
}
