#include "app.hpp"
#include "content_fixture.hpp"
#include "rich_text.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
static App app;
static SaveData before,after;
static unsigned checks,exports;
static bool export_ok=true;
static const char*captures;
static std::string report;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"auxiliary UI8:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(Bytes&b,unsigned at,uint32_t value,unsigned bytes=4){CHECK(at+bytes<=b.size());for(unsigned i=0;i<bytes;++i)b[at+i]=uint8_t(value>>(8*i));}
static Bytes synthetic(){
 // Public fixtures contain arbitrary labels/shapes, never source prose or art.
 constexpr unsigned ni=A_Count,ng=95*5,ns=Ui7StringCount,nc=PhobiaAnimationKinds;
 const unsigned ga=40+ni*16,sa=ga+ng*16,ca=sa+ns*8,base=ca+nc*16+CardKinds*16+HistoryUi8AuxiliaryCount*4;
 const unsigned table=base+64,costs=table+16,descriptors=costs+((ni+1)&~1u);
 Bytes b(descriptors+16);put(b,0,0x3250534e);put(b,4,HistoryUi8ContentVersion);put(b,8,0x41cef1ea);
 put(b,16,ni);put(b,20,ng);put(b,24,ns);put(b,28,nc);put(b,32,CardKinds);
 const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,descriptors,table,770,1,524288,617323,costs,ni};
 for(unsigned i=0;i<16;++i)put(b,base+i*4,fields[i]);
 const unsigned pixel=unsigned(b.size());b.push_back(0);b.push_back(0);
 for(unsigned i=0;i<ni;++i){put(b,40+i*16,pixel);put(b,44+i*16,1,2);put(b,46+i*16,1,2);put(b,52+i*16,2);}
 unsigned glyph=0;for(unsigned encoded:{6u,8u,10u,104u,106u})for(unsigned cp=32;cp<127;++cp,++glyph){
  const unsigned at=ga+glyph*16;put(b,at,cp);put(b,at+4,unsigned(b.size()));put(b,at+8,encoded,2);
  b[at+10]=b[at+11]=1;put(b,at+12,encoded<32?4:5,2);b.push_back(cp==' '?0:255);
 }
 const unsigned empty=unsigned(b.size());b.push_back(0);for(unsigned i=0;i<ns;++i)put(b,sa+i*8,empty);
 const auto text=[&](unsigned slot,const std::string&value){put(b,sa+slot*8,unsigned(b.size()));put(b,sa+slot*8+4,unsigned(value.size()));b.insert(b.end(),value.begin(),value.end());b.push_back(0);};
 text(LocaleMarkerString,"eng");text(AuxiliaryCreditsString,"I0\nHSynthetic project\nRRole\tName\nCShared work\nMOne\nMTwo\nPAlpha\nPBeta\nPGamma\nI1\nI2\nI3\nS476\nI4");
 for(unsigned n=0;n<AuxiliaryNoteCount;++n)for(unsigned k=0;k<AuxiliaryNotes[n].chunks;++k){std::string body="[b]Synthetic note[/b]\n[code]sample.identifier[/code]\n";
  for(unsigned line=0;line<70;++line)body+="Bounded text row "+std::to_string(line)+".\n";text(AuxiliaryNotes[n].text+k,body);}
 for(unsigned i=0;i<nc;++i){put(b,ca+i*16+4,1,2);put(b,ca+i*16+6,33,2);}
 put(b,12,unsigned(b.size()));return b;
}
static bool write_report(const char*bytes,unsigned size){++exports;if(!export_ok)return false;report.assign(bytes,size);return true;}
static void advance(unsigned ms){while(ms){const unsigned step=ms>100?100:ms;app.advance(step);ms-=step;}app.render();}
static void snapshot(){CHECK(save(app.game,before));}
static void unchanged(){CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static HitRegion find(HitKind kind,int value){app.render();for(int i=app.hit_count-1;i>=0;--i)if(app.hits[i].kind==kind&&app.hits[i].value==value)return app.hits[i];CHECK(false);return {};}
static bool exists(HitKind kind,int value){app.render();for(unsigned i=0;i<app.hit_count;++i)if(app.hits[i].kind==kind&&app.hits[i].value==value)return true;return false;}
static void click(HitKind kind,int value){const auto hit=find(kind,value);CHECK(hit.enabled);app.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);app.pointer_button(true);app.pointer_button(false);}
static void press(uint16_t code){CHECK(app.physical_key(code,true));CHECK(app.physical_key(code,false));}
static void capture(const char*name){if(!captures)return;app.render();FILE*f=std::fopen((std::string(captures)+"/"+name+".ppm").c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(uint16_t pixel:app.pixels){uint8_t rgb[]={uint8_t((pixel>>11)*255/31),uint8_t(((pixel>>5)&63)*255/63),uint8_t((pixel&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static void notes(){
 CHECK(AuxiliaryNoteCount==126);unsigned code_scopes=0;for(unsigned n=0;n<AuxiliaryNoteCount;++n){CHECK(AuxiliaryNotes[n].chunks>0&&AuxiliaryNotes[n].chunks<=4);
  CHECK(*app.content.string(AuxiliaryNotes[n].text));if(n)CHECK(AuxiliaryNotes[n-1].date>AuxiliaryNotes[n].date);
  for(unsigned k=0;k<AuxiliaryNotes[n].chunks;++k){const char*text=app.content.string(AuxiliaryNotes[n].text+k);
   const char*start=std::strstr(text,"[code]");if(!start)continue;
   RichReader reader(text);RichChar ch{};std::string plain;
   while(reader.next(ch))plain+=ch.codepoint<128?char(ch.codepoint):'?';
   CHECK(plain.find("[code]")==std::string::npos&&plain.find("[/code]")==std::string::npos);
   while(start){const char*end=std::strstr(start+6,"[/code]");CHECK(end);const std::string body(start+6,end);
    CHECK(!body.empty()&&plain.find(body)!=std::string::npos);++code_scopes;start=std::strstr(end+7,"[code]");}
  }
 }
 CHECK(code_scopes>0);
 snapshot();CHECK(app.physical_key('E',true));CHECK(app.auxiliary.screen==AuxiliaryScreen::None);CHECK(app.physical_key('E',false));
 CHECK(app.auxiliary.screen==AuxiliaryScreen::PatchNotes);advance(250);CHECK(app.auxiliary.scroll.maximum>0);capture("patch-notes-latest");
 CHECK(!exists(HitKind::AuxiliaryControl,AuxiliaryNext));const int menu=app.menu;
 CHECK(app.physical_key(InputLeft,true));CHECK(app.auxiliary.note==0);CHECK(app.physical_key(InputLeft,false));CHECK(app.auxiliary.note==1);
 advance(50);CHECK(app.auxiliary.scroll.offset()==0);press(InputRight);CHECK(app.auxiliary.note==0);
 app.pointer_scroll(5);advance(300);CHECK(app.auxiliary.scroll.offset()>0);const int previous=app.auxiliary.scroll.offset();
 app.pointer_move(170,175);app.pointer_button(true);app.pointer_move(170,100);app.pointer_button(false);advance(200);CHECK(app.auxiliary.scroll.offset()>previous);
 const auto thumb=find(HitKind::AuxiliaryControl,AuxiliaryScrollThumb);app.pointer_move(thumb.x+thumb.w/2,185);app.pointer_button(true);app.pointer_move(thumb.x+thumb.w/2,187);app.pointer_button(false);
 CHECK(app.auxiliary.scroll.position==app.auxiliary.scroll.maximum);capture("patch-notes-scroll-end");
 app.input(Key::Confirm);app.input(Key::EndTurn);app.ui_action(UiAction::OpenCompendium);CHECK(app.auxiliary.screen==AuxiliaryScreen::PatchNotes&&app.menu==menu&&!app.compendium_open);unchanged();
 unsigned multichunk=0;for(unsigned n=0;n<AuxiliaryNoteCount;++n)if(AuxiliaryNotes[n].chunks>1){multichunk=n;break;}
 CHECK(multichunk>0);while(app.auxiliary.note<multichunk)app.auxiliary_control(AuxiliaryPrevious);app.render();
 for(unsigned i=0;i<AuxiliaryNotes[multichunk].chunks;++i)CHECK(app.auxiliary.note_lines[i]>0);capture("patch-notes-multiple-chunks");
 while(app.auxiliary.note+1<AuxiliaryNoteCount)app.auxiliary_control(AuxiliaryPrevious);CHECK(!exists(HitKind::AuxiliaryControl,AuxiliaryPrevious));capture("patch-notes-oldest");
 const unsigned last=app.auxiliary.note;app.auxiliary_control(AuxiliaryPrevious);CHECK(app.auxiliary.note==last);press('E');CHECK(app.auxiliary.closing);advance(249);CHECK(app.auxiliary.screen==AuxiliaryScreen::PatchNotes);advance(1);CHECK(app.auxiliary.screen==AuxiliaryScreen::None);unchanged();
 click(HitKind::AuxiliaryControl,AuxiliaryOpenPatchNotes);advance(250);click(HitKind::AuxiliaryControl,AuxiliaryBack);advance(250);CHECK(app.auxiliary.screen==AuxiliaryScreen::None);
}
static void settings_entries(){
 app.has_save=false;app.menu=1;app.input(Key::Confirm);CHECK(app.settings_open);app.settings_top=3;app.settings_item=7;
 click(HitKind::UiAction,int(UiAction::SettingsCredits));CHECK(app.auxiliary.screen==AuxiliaryScreen::Credits);snapshot();
 app.input(Key::Back);CHECK(!app.auxiliary.closing);advance(1900);app.pointer_button(true);app.pointer_button(false);CHECK(!app.auxiliary.closing);
 advance(100);CHECK(app.auxiliary.credits_position>0);capture("credits-start");const int before_scroll=app.auxiliary.credits_target;
 app.pointer_scroll(2);CHECK(app.auxiliary.credits_target>before_scroll);advance(100);app.auxiliary.credits_position=app.auxiliary.credits_target=400*256;capture("credits-roles");
 CHECK(app.physical_key(27,true)&&!app.auxiliary.closing);CHECK(app.physical_key(27,false)&&app.auxiliary.closing);advance(999);CHECK(app.auxiliary.screen==AuxiliaryScreen::Credits);advance(1);
 CHECK(app.auxiliary.screen==AuxiliaryScreen::None&&app.settings_open&&app.settings_top==3&&app.settings_item==7);unchanged();
 app.settings_item=6;app.input(Key::Confirm);CHECK(app.auxiliary.screen==AuxiliaryScreen::Feedback);capture("feedback-empty");app.close_auxiliary();
 CHECK(app.settings_open&&app.settings_top==3&&app.settings_item==6);click(HitKind::UiAction,int(UiAction::SettingsFeedback));CHECK(app.auxiliary.screen==AuxiliaryScreen::Feedback);
}
static void feedback(){
 auto&v=app.auxiliary;app.pointer.visible=false;app.auxiliary_control(AuxiliaryFeedbackText);CHECK(v.editing);snapshot();
 for(int cp:{int('A'),0x4e2d,0x1f642,int('B')})CHECK(app.auxiliary_text(cp));
 CHECK(v.draft_units==5&&v.draft_size==9);CHECK(std::string(v.draft)=="A中🙂B");
 app.auxiliary_text(-1);app.auxiliary_text(8);CHECK(std::string(v.draft)=="A中B"&&v.draft_units==3&&v.caret==4);
 app.auxiliary_text('X');CHECK(std::string(v.draft)=="A中XB");app.auxiliary_text(127);CHECK(std::string(v.draft)=="A中X");
 app.auxiliary_text(13);app.auxiliary_text('C');CHECK(std::string(v.draft)=="A中X\nC");app.auxiliary_text(-5);CHECK(v.caret==6);app.auxiliary_text(-6);CHECK(v.caret==7);
 app.auxiliary_text(9);CHECK(!v.editing&&v.focus==1);app.input(Key::Confirm);CHECK(v.category_open);capture("feedback-category-list");
 app.auxiliary_control(AuxiliaryFeedbackCategoryFirst+1);CHECK(v.category==1&&!v.category_open);app.auxiliary_control(AuxiliaryFeedbackEmojiFirst+3);CHECK(v.emoji==3);
 app.auxiliary_control(AuxiliaryFeedbackEmojiFirst+3);CHECK(v.emoji==-1);click(HitKind::AuxiliaryControl,AuxiliaryFeedbackEmojiFirst);CHECK(v.emoji==0);capture("feedback-draft");
 app.input(Key::EndTurn);app.input(Key::Inspect);app.input(Key::Map);app.ui_action(UiAction::SettingsBack);CHECK(v.screen==AuxiliaryScreen::Feedback&&app.settings_open);unchanged();
 export_ok=false;const auto draft=std::string(v.draft);click(HitKind::AuxiliaryControl,AuxiliaryFeedbackExport);CHECK(exports==1&&v.export_result==2&&v.draft==draft);capture("feedback-export-failed");
 export_ok=true;click(HitKind::AuxiliaryControl,AuxiliaryFeedbackExport);CHECK(exports==2&&v.export_result==1&&report.find(draft)!=std::string::npos&&report.find("Category: balance")!=std::string::npos&&report.find("Emoji: happy_cultist")!=std::string::npos);capture("feedback-exported");unchanged();
 app.close_auxiliary();advance(60000);app.open_auxiliary(AuxiliaryScreen::Feedback);CHECK(v.draft==draft);app.close_auxiliary();advance(60100);app.open_auxiliary(AuxiliaryScreen::Feedback);CHECK(!v.draft_size&&!v.draft_units&&v.emoji==-1);
 std::memset(v.draft,'a',7998);v.draft[7998]=0;v.draft_size=v.draft_units=v.caret=7998;app.pointer.visible=false;app.auxiliary_control(AuxiliaryFeedbackText);
 app.auxiliary_text(0x1f642);CHECK(v.draft_units==8000&&v.draft_size==8002);app.auxiliary_text('b');CHECK(v.draft_units==8000&&v.draft_size==8002);
 app.auxiliary_text(8);CHECK(v.draft_size==7998&&v.draft_units==7998);app.auxiliary_text(0x4e2d);app.auxiliary_text(0x4e2d);CHECK(v.draft_units==8000&&v.draft_size==8004);
 app.auxiliary_text(0xd800);app.auxiliary_text(0x110000);CHECK(v.draft_units==8000);char small[32];CHECK(!app.feedback_report(small,sizeof small));
 app.auxiliary_text(27);CHECK(!v.editing&&v.screen==AuxiliaryScreen::Feedback);app.input(Key::Back);CHECK(v.screen==AuxiliaryScreen::None&&app.settings_top==3);unchanged();
}
}
int main(int argc,char**argv){
 Bytes pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);}else pack=synthetic();
 if(argc>2)captures=argv[2];CHECK(app.content.open(pack.data(),pack.size())&&app.content.current_profile());app.callbacks.write_feedback=write_report;
 new_run(app.game,0x1617,false);app.title=true;app.input_bindings=default_input_bindings();notes();settings_entries();feedback();
 CHECK(app.content.healthy());std::printf("Auxiliary UI8: %u checks; 126 complete notes, released hotkeys, scrolling, credits timing, settings return, Unicode editing and local report export passed\n",checks);
}
