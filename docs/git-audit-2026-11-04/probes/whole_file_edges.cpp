#include "Test.h"
#include "git/Cli.h"
#include "git/Config.h"
#include "git/Index.h"
#include "git/Commit.h"
#include "git/Tree.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/DiffModel.h"
#include <QFile>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
void write(const QString &p, const QByteArray &c) { QFile f(p); if(!f.open(QFile::WriteOnly)||f.write(c)!=c.size()) std::abort(); }
git::RunResult run(const QString &d,const QStringList &a) { auto r=git::Cli(d).run(a); if(r.exitCode){fprintf(stderr,"git failed %s\n",r.errors.constData());std::abort();}return r; }
QByteArray text(bool changed,bool external=false) { QByteArray b;for(int i=1;i<=30;++i)b+=(external&&i==15?QByteArray("EXTERNAL-15"):(changed&&(i==5||i==25)?QByteArray("changed-")+QByteArray::number(i):QByteArray("line-")+QByteArray::number(i)))+'\n';return b; }
void staleDiscard(bool whole) {
 Test::ScratchRepository r; git::Repository repo=r; QString dir=repo.workdir().path();
 write(dir+"/file.txt",text(false));run(dir,{"add","."});run(dir,{"commit","-qm","base"});
 write(dir+"/file.txt",text(true)); auto index=repo.index();index.read();auto diff=repo.status(index,nullptr,false);
 MainWindow window(repo); DiffModel model(window.currentView()); model.setDiff(diff,"file.txt");model.setWholeFile(whole);
 int h=0;QList<bool> selected(model.lines(h).size(),false);for(int i=0;i<selected.size();++i)if(model.lines(h)[i].content=="line-5\n"||model.lines(h)[i].content=="changed-5\n")selected[i]=true;
 write(dir+"/file.txt",text(true,true));model.discard(h,selected);
 QFile f(dir+"/file.txt");if(!f.open(QFile::ReadOnly))std::abort();auto content=f.readAll();
 fprintf(stderr,"STALE_DISCARD whole=%d external_preserved=%d selected_reverted=%d other_change_preserved=%d\n",whole,content.contains("EXTERNAL-15"),content.contains("line-5\n"),content.contains("changed-25"));
}
void modeOnly() {
 Test::ScratchRepository r;git::Repository repo=r;QString dir=repo.workdir().path();write(dir+"/file.txt","one\ntwo\n");run(dir,{"add","."});run(dir,{"commit","-qm","base"});run(dir,{"update-index","--chmod=+x","file.txt"});
 auto index=repo.index();index.read();auto diff=repo.status(index,nullptr,false); MainWindow w(repo);DiffModel model(w.currentView());model.setDiff(diff,"file.txt");model.setWholeFile(true);
 fprintf(stderr,"MODE_ONLY whole=%d allowed=%d rows=%d notice=%s\n",model.isWholeFile(),model.canShowWholeFile(),model.rowCount(),model.notice().toUtf8().constData());
}
void symlink(bool doStage=false, bool whole=true) {
 Test::ScratchRepository r;git::Repository repo=r;QString dir=repo.workdir().path();write(dir+"/a.txt","AAA\n");write(dir+"/b.txt","BBB\nCCC\n");::symlink("a.txt",(dir+"/link").toUtf8().constData());run(dir,{"add","."});run(dir,{"commit","-qm","base"});QFile::remove(dir+"/link");::symlink("b.txt",(dir+"/link").toUtf8().constData());
 auto index=repo.index();index.read();auto diff=repo.status(index,nullptr,false);MainWindow w(repo);DiffModel model(w.currentView());model.setDiff(diff,"link");QByteArray before;for(int h=0;h<model.hunkCount();++h)for(auto l:model.lines(h))before+=l.content;model.setWholeFile(whole);QByteArray after;for(int h=0;h<model.hunkCount();++h)for(auto l:model.lines(h))after+=l.content;
 fprintf(stderr,"SYMLINK before_hex=%s whole_hex=%s whole=%d allowed=%d\n",before.toHex().constData(),after.toHex().constData(),model.isWholeFile(),model.canShowWholeFile());
 if (doStage) {
  int row=-1; for(int h=0;h<model.hunkCount();++h) for(int l=0;l<model.lines(h).size();++l) if(row<0 && model.lines(h)[l].origin=='+')row=model.row(h,l);
  fprintf(stderr,"SYMLINK_PARTIAL_STAGE whole=%d row=%d starting=1\n",whole,row);
  model.toggleLine(row); for(int i=0;i<10;++i)QTest::qWait(10);
  auto staged=run(dir,{"show",":link"});fprintf(stderr,"SYMLINK_PARTIAL_STAGE returned=1 staged_hex=%s\n",staged.output.toHex().constData());
 }
}
int main(int argc,char**argv){QByteArray option=argc>1?argv[1]:"";Application::setInTest();auto app=Test::createApp(argc,argv);Test::ignoreSystemConfig();if(!option.isEmpty()){symlink(true,option=="whole");return 0;}staleDiscard(false);staleDiscard(true);modeOnly();symlink();}
