#include "git/Repository.h"
#include "git/Reference.h"
#include "git/Config.h"
#include "git/Signature.h"
#include "git/Rewrite.h"
#include "git2.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>
#include <iostream>
struct Out {int code;QByteArray out,err;};
static Out cli(const QString&p,const QStringList&a){QProcess x;x.setWorkingDirectory(p);x.start("git",a);if(!x.waitForFinished())exit(2);return {x.exitCode(),x.readAllStandardOutput().trimmed(),x.readAllStandardError().trimmed()};}
static QByteArray gitRun(const QString&p,const QStringList&a){auto r=cli(p,a);if(r.code){std::cerr<<a.join(' ').toStdString()<<":"<<r.err.constData()<<'\n';exit(3);}return r.out;}
static void put(const QString&p,const QByteArray&d,bool exe=false){QFile f(p);if(!f.open(QFile::WriteOnly)||f.write(d)!=d.size())exit(4);f.close();if(exe)QFile::setPermissions(p,QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);}
static git::Repository init(const QString&p){auto r=git::Repository::init(p);r.gitConfig().setValue("user.name",QString("Review"));r.gitConfig().setValue("user.email",QString("review@example.test"));r.gitConfig().setValue("commit.gpgsign",false);return r;}
static git::Commit add(git::Repository&r,const QString&n){QString p=r.workdir().path();put(p+"/"+n,n.toUtf8()+"\n");gitRun(p,{"add",n});gitRun(p,{"commit","-qm",n});r.index().read();return r.head().target();}
static void signConfig(git::Repository&r,const QString&script){QString p=r.workdir().path(),key=p+"/.git/key";QProcess k;k.start("ssh-keygen",{"-q","-t","ed25519","-N","","-f",key});if(!k.waitForFinished()||k.exitCode())exit(5);put(p+"/.git/signer",script.toUtf8(),true);auto c=r.gitConfig();c.setValue("commit.gpgsign",true);c.setValue("gpg.format",QString("ssh"));c.setValue("user.signingkey",key);c.setValue("gpg.ssh.program",p+"/.git/signer");}
int main(int ac,char**av){QCoreApplication app(ac,av);qputenv("GIT_CONFIG_GLOBAL","/dev/null");qputenv("GIT_CONFIG_NOSYSTEM","1");git::Repository::init();
 {QTemporaryDir t;auto r=init(t.path());auto old=add(r,"a"),newer=add(r,"b");bool ok=r.amend(old,r.defaultSignature(),r.defaultSignature(),"stale");std::cout<<"STALE_AMEND accepted="<<ok<<" newer_preserved="<<(r.head().target().id()==newer.id())<<'\n';}
 {QTemporaryDir t;auto r=init(t.path());auto b=add(r,"a");gitRun(t.path(),{"commit","-q","--allow-empty","-m","marker"});auto e=r.head().target();git::Rewrite rw(r);auto x=rw.apply(b,{{e,git::Rewrite::Reword,"renamed marker"}},r.defaultSignature());std::cout<<"EMPTY_REWORD valid="<<x.isValid()<<" result_is_base="<<(x.id()==b.id())<<" message="<<x.summary().toStdString()<<" parent_is_base="<<(x.parents().value(0).id()==b.id())<<'\n';}
 {QTemporaryDir t;auto r=init(t.path());auto b=add(r,"a");auto concurrent=gitRun(t.path(),{"commit-tree",b.id().toString()+"^{tree}","-p",b.id().toString(),"-m","external"});QString script="#!/bin/sh\ngit -C '"+t.path()+"' update-ref HEAD "+QString::fromUtf8(concurrent)+" || exit 10\nexec /usr/bin/ssh-keygen \"$@\"\n";signConfig(r,script);put(t.path()+"/signed","signed\n");gitRun(t.path(),{"add","signed"});r.index().read();auto x=r.commit("signed");std::cout<<"SIGNED_NEW_TIP accepted="<<x.isValid()<<" concurrent_preserved="<<(r.head().target().id().toString().toUtf8()==concurrent)<<'\n';}
 {QTemporaryDir t;auto r=init(t.path());auto b=add(r,"a");QString original=r.head().qualifiedName();gitRun(t.path(),{"branch","other"});QString script="#!/bin/sh\ngit -C '"+t.path()+"' symbolic-ref HEAD refs/heads/other || exit 10\nexec /usr/bin/ssh-keygen \"$@\"\n";signConfig(r,script);put(t.path()+"/signed","signed\n");gitRun(t.path(),{"add","signed"});r.index().read();auto x=r.commit("signed");std::cout<<"PROJECT_SAME_TIP_SWITCH accepted="<<x.isValid()<<" head="<<r.head().qualifiedName().toStdString()<<" original_preserved="<<(r.lookupRef(original).target().id()==b.id())<<" other_moved="<<(r.lookupRef("refs/heads/other").target().id()!=b.id())<<'\n';
 gitRun(t.path(),{"update-ref","refs/heads/other",b.id().toString()});gitRun(t.path(),{"symbolic-ref","HEAD",original});auto native=cli(t.path(),{"commit","-qm","native signed"});std::cout<<"NATIVE_SAME_TIP_SWITCH exit="<<native.code<<" head="<<gitRun(t.path(),{"symbolic-ref","HEAD"}).constData()<<" original_preserved="<<(gitRun(t.path(),{"rev-parse",original})==b.id().toString().toUtf8())<<" other_moved="<<(gitRun(t.path(),{"rev-parse","refs/heads/other"})!=b.id().toString().toUtf8())<<'\n';}
 git::Repository::shutdown();
}
