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

const BindingFlags all=BindingFlags.Static|BindingFlags.Instance|BindingFlags.Public|BindingFlags.NonPublic;
CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;CultureInfo.CurrentUICulture=CultureInfo.InvariantCulture;

Type T(string name)=>assembly.GetType("MegaCrit.Sts2.Core."+name,true)!;
object alloc(Type t)=>System.Runtime.CompilerServices.RuntimeHelpers.GetUninitializedObject(t);
var db=T("Models.ModelDb");var dict=(System.Collections.IDictionary)db.GetField("_contentById",all)!.GetValue(null)!;
foreach(var name in new[]{"Acts.Overgrowth","Acts.Underdocks","Acts.Hive","Acts.Glory","Characters.Ironclad","Characters.Silent","Characters.Regent","Characters.Necrobinder","Characters.Defect"}){
 var t=T("Models."+name);var model=Activator.CreateInstance(t,true)!;var id=t.GetProperty("Id",all)!.GetValue(model)!;dict.Add(id,model);Console.WriteLine("REGISTER "+id);
}
var progress=Activator.CreateInstance(T("Saves.ProgressState"),true)!;
var pm=alloc(T("Saves.Managers.ProgressSaveManager"));pm.GetType().GetField("<Progress>k__BackingField",all)!.SetValue(pm,progress);
var save=alloc(T("Saves.SaveManager"));save.GetType().GetField("_progressSaveManager",all)!.SetValue(save,pm);save.GetType().GetField("_mockInstance",all)!.SetValue(null,save);
var unlockType=T("Unlocks.UnlockState");var modelId=T("Models.ModelId");
var ctor=unlockType.GetConstructors(all).Single(x=>x.GetParameters().Length==3);

var rngType=T("Random.Rng");var actType=T("Models.ActModel");var characterType=T("Models.CharacterModel");
var ids=new string[]{"ACT2_B_EPOCH","ACT3_B_EPOCH","COLORLESS1_EPOCH","COLORLESS2_EPOCH","COLORLESS3_EPOCH","COLORLESS4_EPOCH","COLORLESS5_EPOCH","CUSTOM_AND_SEEDS_EPOCH","DAILY_RUN_EPOCH","DARV_EPOCH","DEFECT1_EPOCH","DEFECT2_EPOCH","DEFECT3_EPOCH","DEFECT4_EPOCH","DEFECT5_EPOCH","DEFECT6_EPOCH","DEFECT7_EPOCH","EVENT1_EPOCH","EVENT2_EPOCH","EVENT3_EPOCH","IRONCLAD2_EPOCH","IRONCLAD3_EPOCH","IRONCLAD4_EPOCH","IRONCLAD5_EPOCH","IRONCLAD6_EPOCH","IRONCLAD7_EPOCH","NECROBINDER1_EPOCH","NECROBINDER2_EPOCH","NECROBINDER3_EPOCH","NECROBINDER4_EPOCH","NECROBINDER5_EPOCH","NECROBINDER6_EPOCH","NECROBINDER7_EPOCH","NEOW_EPOCH","OROBAS_EPOCH","POTION1_EPOCH","POTION2_EPOCH","REGENT1_EPOCH","REGENT2_EPOCH","REGENT3_EPOCH","REGENT4_EPOCH","REGENT5_EPOCH","REGENT6_EPOCH","REGENT7_EPOCH","RELIC1_EPOCH","RELIC2_EPOCH","RELIC3_EPOCH","RELIC4_EPOCH","RELIC5_EPOCH","SILENT1_EPOCH","SILENT2_EPOCH","SILENT3_EPOCH","SILENT4_EPOCH","SILENT5_EPOCH","SILENT6_EPOCH","SILENT7_EPOCH","UNDERDOCKS_EPOCH"};
var known=progress.GetType().GetField("_discoveredActs",all)!.GetValue(progress)!;
var knownClear=known.GetType().GetMethod("Clear")!;var knownAdd=known.GetType().GetMethod("Add")!;
var getActs=actType.GetMethod("GetRandomList",all)!;
var canonicalize=T("Helpers.SeedHelper").GetMethod("CanonicalizeSeed",all)!;
var hash=T("Helpers.StringHelper").GetMethod("GetDeterministicHashCode",all)!;
var characters=db.GetProperty("AllCharacters",all)!.GetValue(null)!;
var choose=rngType.GetMethods(all).Single(m=>m.Name=="NextItem"&&m.IsGenericMethod&&m.GetParameters().Length==1).MakeGenericMethod(characterType);
var outputLines=new List<string>{"# Original v0.111.0 / 41cef1ea GetRandomList + gated Random; case input canonical root region character calls state0 state1 state2 state3"};
foreach(string scenario in new[]{"fresh","legacy_seen","all_unseen","underdocks_unseen","underdocks_seen"}){
 knownClear.Invoke(known,null);
 if(scenario.EndsWith("_seen"))foreach(System.Collections.DictionaryEntry pair in dict)if(pair.Value!.GetType().BaseType==actType)knownAdd.Invoke(known,new[]{pair.Key});
 var granted=scenario=="fresh"?Array.Empty<string>():scenario.StartsWith("underdocks")?new[]{"UNDERDOCKS_EPOCH"}:ids;
 var unlock=ctor.Invoke(new object[]{granted,Array.CreateInstance(modelId,0),scenario=="fresh"?0:1});
 var available=((System.Collections.IEnumerable)unlockType.GetProperty("Characters",all)!.GetValue(unlock)!).Cast<object>().Count();
 foreach(string seed in new[]{"oldA","A","B","C","D","UI24 TIMELINE","SEED64-0000000000000000","1234567890"}){
  var canonical=(string)canonicalize.Invoke(null,new object[]{seed})!;var root=(ulong)hash.Invoke(null,new object[]{canonical})!;
  var rng=rngType.GetConstructor(new[]{typeof(ulong),typeof(string)})!.Invoke(new object[]{root,"act_selection"});
  var acts=((System.Collections.IEnumerable)getActs.Invoke(null,new object[]{rng,unlock,false})!).Cast<object>().Select(x=>x.GetType().Name).ToArray();
  if(acts.Length!=3||acts[1]!="Hive"||acts[2]!="Glory")throw new InvalidDataException("Changed source Act buckets");
  int character=-1;if(available==5){var choice=choose.Invoke(rng,new[]{characters})!;character=Array.IndexOf(new[]{"Ironclad","Silent","Defect","Regent","Necrobinder"},choice.GetType().Name);}
  var random=rngType.GetField("_random",all)!.GetValue(rng)!;
  var fields=random.GetType().GetFields(all).Where(f=>f.FieldType==typeof(ulong)).ToArray();if(fields.Length!=4)throw new InvalidDataException("Unexpected source RNG words");
  outputLines.Add(string.Join("\t",new[]{scenario,seed,canonical,root.ToString("X16"),(acts[0]=="Underdocks"?1:0).ToString(),character.ToString(),rngType.GetField("_counter",all)!.GetValue(rng)!.ToString()!}.Concat(fields.Select(f=>((ulong)f.GetValue(random)!).ToString("X16")))));
 }
}
File.WriteAllLines(Path.Combine(output,"startup-vectors.tsv"),outputLines);
var audited=new[]{getActs,canonicalize,hash,choose.GetGenericMethodDefinition()}.ToDictionary(m=>m.DeclaringType!.FullName+"."+m.Name,m=>new{token=m.MetadataToken.ToString("X8"),il_sha256=digest(m.GetMethodBody()!.GetILAsByteArray()!)});
File.WriteAllText(Path.Combine(output,"oracle-receipt.json"),JsonSerializer.Serialize(new{reference="v0.111.0 / 41cef1ea",assembly_sha256=expected,bootstrap="Only the telemetry module initializer is disabled; original Act/Character constructors populate ModelDb. SaveManager mock wraps a real ProgressState; TestMode remains off.",vectors=outputLines.Count-1,methods=audited},new JsonSerializerOptions{WriteIndented=true}));
Console.WriteLine(string.Join("\n",outputLines));
