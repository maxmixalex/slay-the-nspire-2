// Optional pinned-reference calendar oracle. Original assembly/resources stay
// private. Only the original culture map and .NET formatting are executed.
using System.Globalization;
using System.Reflection;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Reflection.PortableExecutable;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Runtime.CompilerServices;
using System.Text.Encodings.Web;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

if(args.Length!=3)throw new ArgumentException("ASSEMBLY_DIRECTORY PRIVATE_FORMATS_JSON PRIVATE_OUTPUT_DIRECTORY");
string source=Path.GetFullPath(args[0]),output=Path.GetFullPath(args[2]);
if(source==output)throw new ArgumentException("Keep the original assembly directory separate and immutable");
byte[] original=File.ReadAllBytes(Path.Combine(source,"sts2.dll"));
const string expected="0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9";
string digest(byte[] bytes)=>Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();
if(digest(original)!=expected)throw new InvalidDataException("Expected v0.111.0 / 41cef1ea sts2.dll");
Directory.CreateDirectory(output);string isolated=Path.Combine(output,"isolated-assembly");Directory.CreateDirectory(isolated);
byte[] copy=(byte[])original.Clone();int offset=-1;bool cultureAudited=false;
using(var pe=new PEReader(new MemoryStream(original))){
 var metadata=pe.GetMetadataReader();
 foreach(var h in metadata.TypeDefinitions){
  var type=metadata.GetTypeDefinition(h);string name=metadata.GetString(type.Name);
  if(name!="<Module>"&&name!="NGame")continue;
  foreach(var mh in type.GetMethods()){
   var method=metadata.GetMethodDefinition(mh);string methodName=metadata.GetString(method.Name);
   if(name=="<Module>"&&methodName==".cctor"){
    int rva=method.RelativeVirtualAddress;
    var section=pe.PEHeaders.SectionHeaders.Single(s=>rva>=s.VirtualAddress&&rva<s.VirtualAddress+s.VirtualSize);
    offset=section.PointerToRawData+rva-section.VirtualAddress;
   }
   if(name=="NGame"&&methodName=="_EnterTree"){
    byte[] il=pe.GetMethodBody(method.RelativeVirtualAddress).GetILBytes()!;
    string[] calls={"get_InvariantCulture","set_DefaultThreadCurrentCulture","get_InvariantCulture","set_DefaultThreadCurrentUICulture"};
    for(int i=0;i<calls.Length;i++){
     if(il[i*5]!=0x28)throw new InvalidDataException("Original culture initialization changed");
     var member=metadata.GetMemberReference((MemberReferenceHandle)MetadataTokens.Handle(BitConverter.ToInt32(il,i*5+1)));
     if(metadata.GetString(member.Name)!=calls[i])throw new InvalidDataException("Original culture initialization changed");
    }
    cultureAudited=true;
   }
  }
 }
}
if(!cultureAudited)throw new InvalidDataException("Original culture initialization was not found");
if(offset<0||original[offset]!=0x1a||original[offset+1]!=0x28)throw new InvalidDataException("Unexpected module initializer");
// Its tiny body only calls SentryAutoInit. Prevent telemetry/native Godot
// startup; retain every localization/history/daily method's original bytes and metadata.
copy[offset]=0x06;copy[offset+1]=0x2a;
File.WriteAllBytes(Path.Combine(isolated,"sts2.dll"),copy);
foreach(string name in new[]{"GodotSharp.dll","System.IO.Hashing.dll","Sentry.dll","SmartFormat.dll","SmartFormat.ZString.dll"})File.Copy(Path.Combine(source,name),Path.Combine(isolated,name),true);
AssemblyLoadContext.Default.Resolving+=(ctx,name)=>{
 string path=Path.Combine(isolated,name.Name+".dll");return File.Exists(path)?ctx.LoadFromAssemblyPath(path):null;
};
var assembly=AssemblyLoadContext.Default.LoadFromAssemblyPath(Path.Combine(isolated,"sts2.dll"));
const BindingFlags flags=BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic;
var type=assembly.GetType("MegaCrit.Sts2.Core.Localization.LocManager",true)!;
var mapper=type.GetMethod("CultureInfoFromThreeLetterCode",flags|BindingFlags.Instance)!;
if(mapper.MetadataToken!=0x06004aad)throw new InvalidDataException("Original culture method changed");
// No SaveManager/Godot constructor or game scene is invoked. The unchanged
// mapper only uses its static table for these supported language identifiers.
var manager=RuntimeHelpers.GetUninitializedObject(type);
CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;
CultureInfo.CurrentUICulture=CultureInfo.InvariantCulture;
using var formats=JsonDocument.Parse(File.ReadAllBytes(args[1]));
string[] codes={"eng","zhs","zht","deu","esp","fra","ind","ita","jpn","kor","pol","ptb","rus","spa","tha","tur"};
var samples=new SortedSet<uint>{0,1,86400,951782400,951825600,1709164800,4107542400,UInt32.MaxValue};
foreach(int year in new[]{1970,2000,2024,2026,2099,2100,2106})
 foreach(int month in Enumerable.Range(1,12))
  foreach(int hour in new[]{0,1,11,12,13,23}){
   var date=new DateTimeOffset(year,month,24,hour,7,0,TimeSpan.Zero);
   long timestamp=date.ToUnixTimeSeconds();if(timestamp<=UInt32.MaxValue)samples.Add((uint)timestamp);
  }
var compact=new HashSet<uint>();
foreach(int hour in new[]{0,12,23})compact.Add((uint)new DateTimeOffset(2026,10,24,hour,7,0,TimeSpan.Zero).ToUnixTimeSeconds());
compact.Add(951825600);samples.UnionWith(compact);
var metadata=new List<object>();int count=0;
string hex(string value)=>Convert.ToHexString(Encoding.UTF8.GetBytes(value));
string vectors=Path.Combine(output,"calendar-original.tsv"),small=Path.Combine(output,"calendar-synthetic-ui18.tsv");
using(var writer=new StreamWriter(vectors){NewLine="\n"})
using(var fixture=new StreamWriter(small){NewLine="\n"}){
 writer.WriteLine("# language timestamp date-pattern time-pattern daily-pattern date time daily (UTF-8 hexadecimal)");
 fixture.WriteLine("# Synthetic dates; public culture facts, no original resource bytes or glyph payloads.");
 for(int i=0;i<codes.Length;i++){
  string code=codes[i];var culture=(CultureInfo)mapper.Invoke(manager,new object[]{code})!;var dtf=culture.DateTimeFormat;
  var record=formats.RootElement.GetProperty(code);
  string df=record.GetProperty("date_format").GetString()!,tf=record.GetProperty("time_format").GetString()!,daily=record.GetProperty("daily_date_format").GetString()!;
  metadata.Add(new{code,culture=culture.Name,calendar=dtf.Calendar.GetType().Name,year=dtf.Calendar.GetYear(new DateTime(2026,1,1)),months=dtf.MonthNames,genitive=dtf.MonthGenitiveNames,am=dtf.AMDesignator,pm=dtf.PMDesignator,date_separator=dtf.DateSeparator,time_separator=dtf.TimeSeparator});
  foreach(uint timestamp in samples){
   // The platform time-zone adapter is a separately retained boundary. Use
   // identical UTC calendar fields to isolate the original format/provider.
   DateTime value=DateTimeOffset.FromUnixTimeSeconds(timestamp).UtcDateTime;
   string row=String.Join("\t",new object[]{i+1,timestamp,hex(df),hex(tf),hex(daily),hex(value.ToString(df,dtf)),hex(value.ToString(tf,dtf)),hex(new DateTimeOffset(value).ToString(daily))});
   writer.WriteLine(row);if(compact.Contains(timestamp))fixture.WriteLine(row);++count;
  }
 }
}
var jsonOptions=new JsonSerializerOptions{WriteIndented=true,Encoder=JavaScriptEncoder.UnsafeRelaxedJsonEscaping};
File.WriteAllText(Path.Combine(output,"cultures.json"),JsonSerializer.Serialize(metadata,jsonOptions)+"\n");
var receipt=new{
 reference="v0.111.0 / 41cef1ea",original_sha256=expected,isolated_sha256=digest(copy),
 isolation=new{changed_offsets=new[]{offset,offset+1},purpose="Disable only Sentry module initializer; all localization/history/daily IL unchanged"},
 original_mapper_token="0x06004aad",history_method_token="0x0600703f",daily_method_token="0x060077a2",
 runtime=RuntimeInformation.FrameworkDescription,os=RuntimeInformation.OSDescription,
 current_culture=CultureInfo.CurrentCulture.Name,local_zone=TimeZoneInfo.Local.Id,
 sort_version=CultureInfo.InvariantCulture.CompareInfo.Version.FullVersion,
 globalization_invariant=AppContext.TryGetSwitch("System.Globalization.Invariant",out bool invariant)&&invariant,
 globalization_use_nls=AppContext.TryGetSwitch("System.Globalization.UseNls",out bool nls)&&nls,
 time_basis="same UTC fields; tests formatting/provider, not calculator timezone conversion",
 cultures=codes.Length,samples=samples.Count,rows=count,
 formats_sha256=digest(File.ReadAllBytes(args[1])),vectors_sha256=digest(File.ReadAllBytes(vectors)),
 compact_sha256=digest(File.ReadAllBytes(small))
};
File.WriteAllText(Path.Combine(output,"receipt.json"),JsonSerializer.Serialize(receipt,jsonOptions)+"\n");
Console.WriteLine(JsonSerializer.Serialize(receipt));
