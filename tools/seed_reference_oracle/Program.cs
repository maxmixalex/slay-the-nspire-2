// Optional private-reference oracle. No original assembly, IL or resource is
// distributed with this tool. All generated material belongs outside Git.
using System.Globalization;
using System.Reflection;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Reflection.PortableExecutable;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

if(args.Length!=2)throw new ArgumentException("ASSEMBLY_DIRECTORY PRIVATE_OUTPUT_DIRECTORY");
string source=Path.GetFullPath(args[0]),output=Path.GetFullPath(args[1]);
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
// startup; retain every seed/hash/RNG method's original bytes and metadata.
copy[offset]=0x06;copy[offset+1]=0x2a;
File.WriteAllBytes(Path.Combine(isolated,"sts2.dll"),copy);
foreach(string name in new[]{"GodotSharp.dll","System.IO.Hashing.dll"})File.Copy(Path.Combine(source,name),Path.Combine(isolated,name),true);
AssemblyLoadContext.Default.Resolving+=(ctx,name)=>{
 string path=Path.Combine(isolated,name.Name+".dll");return File.Exists(path)?ctx.LoadFromAssemblyPath(path):null;
};
var assembly=AssemblyLoadContext.Default.LoadFromAssemblyPath(Path.Combine(isolated,"sts2.dll"));
const BindingFlags flags=BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic;
var helper=assembly.GetType("MegaCrit.Sts2.Core.Helpers.SeedHelper",true)!;
var strings=assembly.GetType("MegaCrit.Sts2.Core.Helpers.StringHelper",true)!;
var rng=assembly.GetType("MegaCrit.Sts2.Core.Runs.RunRngSet",true)!;
var canonical=helper.GetMethod("CanonicalizeSeed",flags)!.CreateDelegate<Func<string,string>>();
var hash=strings.GetMethod("GetDeterministicHashCode",flags)!.CreateDelegate<Func<string,ulong>>();
var ctor=rng.GetConstructor(new[]{typeof(string)})!;var seed=rng.GetProperty("Seed")!;
CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;CultureInfo.CurrentUICulture=CultureInfo.InvariantCulture;
string scalarPath=Path.Combine(output,"scalar-original.tsv"),prefixPath=Path.Combine(output,"prefix-invariant.tsv"),vectorPath=Path.Combine(output,"seed-original-vectors.tsv");
var prefixPoints=new List<int>();int scalarCount=0;
using(var scalar=new StreamWriter(scalarPath){NewLine="\n"})
using(var prefix=new StreamWriter(prefixPath){NewLine="\n"}){
 for(int cp=1;cp<=0x10ffff;cp++){
  if(cp>=0xd800&&cp<=0xdfff)continue;
  string text=char.ConvertFromUtf32(cp),value=canonical(text);++scalarCount;
  if(text!=value)scalar.WriteLine($"{cp:X}\t{Convert.ToHexString(Encoding.UTF8.GetBytes(value))}");
  // These are exactly the invariant-culture operation and overload called
  // by the original constructor; the vector oracle below executes that
  // constructor too, including multi-scalar contexts and UTF-16 slicing.
  int bits=0;
  if((text+"old").StartsWith("old"))bits|=1;
  if((text+"ld").StartsWith("old"))bits|=2;
  if(("o"+text+"d").StartsWith("old"))bits|=4;
  if(("ol"+text).StartsWith("old"))bits|=8;
  if(!("old"+text).StartsWith("old"))bits|=16;
  if(bits!=0){prefix.WriteLine($"{cp:X}\t{bits:X}");prefixPoints.Add(cp);}
 }
}
var inputs=new List<string>{""," ","old","oldA","olda","OLDa","oldabc","oldé","old𝔞😀","old\u0301I o"," oId ","ıiİIſßﬃ\u1f80", "é", "e\u0301", "\u00a0\u3000éio\u202f", "\u200btest\ufeff", "\U00010428\U0001e922", "o\u00adldA", "\u00adoldA", "öłđ", "old😀", "old𐐨", "ＡｏＩ", "old\U000e0100A", "\U000e0100oldA", "\U000e0100\U000e0100oldA", "old\u00ad\u0301A", "ol\u00addA", "old\u0301", "old\u034fA"};
var pool=new List<string>{"o","l","d","O","L","D","a","x"," ","\t","\n","😀","中","ß","é","e\u0301"};
foreach(int cp in prefixPoints){
 string s=char.ConvertFromUtf32(cp);pool.Add(s);
 inputs.Add(s+"oldA");inputs.Add("o"+s+"ldA");inputs.Add("ol"+s+"dA");inputs.Add("old"+s+"A");
 inputs.Add("old"+s+"\u0301");inputs.Add("old\u00ad"+s);inputs.Add("\U000e0100"+s+"oldA");
}
var random=new Random(0x0175eed);
for(int i=0;i<20000;i++){
 var b=new StringBuilder();int n=random.Next(1,10);
 switch(i%5){case 0:b.Append("old");break;case 1:b.Append("ol");break;case 2:b.Append("o");break;case 3:b.Append("\u00adold");break;}
 for(int j=0;j<n;j++)b.Append(pool[random.Next(pool.Count)]);
 if(i%7==0)b.Append("old");inputs.Add(b.ToString());
}
using(var vectors=new StreamWriter(vectorPath){NewLine="\n"}){
 foreach(string text in inputs){
  string normalized=canonical(text);string hex(string s)=>s.Length==0?"-":Convert.ToHexString(Encoding.UTF8.GetBytes(s));
  ulong raw=Convert.ToUInt64(seed.GetValue(ctor.Invoke(new object[]{text})));
  vectors.WriteLine($"{hex(text)}\t{hex(normalized)}\t{raw:X16}\t{hash(normalized):X16}");
 }
}
var receipt=new{
 reference="v0.111.0 / 41cef1ea",original_sha256=expected,isolated_sha256=digest(copy),
 isolation=new{changed_offsets=new[]{offset,offset+1},purpose="Disable only Sentry module initializer; seed/hash/RNG IL unchanged"},
 runtime=RuntimeInformation.FrameworkDescription,os=RuntimeInformation.OSDescription,
 culture=CultureInfo.CurrentCulture.Name,sort_version=CultureInfo.InvariantCulture.CompareInfo.Version.FullVersion,
 globalization_invariant=AppContext.TryGetSwitch("System.Globalization.Invariant",out bool invariant)&&invariant,
 globalization_use_nls=AppContext.TryGetSwitch("System.Globalization.UseNls",out bool nls)&&nls,
 scalar_count=scalarCount,vector_count=inputs.Count,
 scalar_sha256=digest(File.ReadAllBytes(scalarPath)),prefix_sha256=digest(File.ReadAllBytes(prefixPath)),vectors_sha256=digest(File.ReadAllBytes(vectorPath))
};
File.WriteAllText(Path.Combine(output,"receipt.json"),JsonSerializer.Serialize(receipt,new JsonSerializerOptions{WriteIndented=true})+"\n");
Console.WriteLine(JsonSerializer.Serialize(receipt));
