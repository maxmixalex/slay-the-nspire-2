// Optional private assembly execution. The output contains original-method
// facts, never assembly bytes or decompiled methods, and belongs outside Git.
using System.Collections;
using System.Globalization;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

static class AsciiOracle {
 const BindingFlags All=BindingFlags.Static|BindingFlags.Instance|BindingFlags.Public|BindingFlags.NonPublic;
 static string Hex(string s)=>s.Length==0?"-":Convert.ToHexString(Encoding.UTF8.GetBytes(s));
 public static void Run(Assembly assembly,string output){
  var helper=assembly.GetType("MegaCrit.Sts2.Core.Helpers.SeedHelper",true)!;
  var runType=assembly.GetType("MegaCrit.Sts2.Core.Runs.RunRngSet",true)!;
  var randomType=assembly.GetType("MegaCrit.Sts2.Core.Random.Rng",true)!;
  var megaType=assembly.GetType("MegaCrit.Sts2.Core.Random.MegaRandom",true)!;
  var mapType=assembly.GetType("MegaCrit.Sts2.Core.Map.StandardActMap",true)!;
  var canonical=helper.GetMethod("CanonicalizeSeed",All)!.CreateDelegate<Func<string,string>>();
  var runCtor=runType.GetConstructor(new[]{typeof(string)})!;
  var namedCtor=randomType.GetConstructor(new[]{typeof(ulong),typeof(string)})!;
  var generate=helper.GetMethod("GetRandomSeed",All)!;
  var next=randomType.GetMethod("NextUnsignedLong",Type.EmptyTypes)!;
  var rngField=randomType.GetField("_random",All)!;
  var countField=randomType.GetField("_counter",All)!;
  var words=Enumerable.Range(0,4).Select(i=>megaType.GetField("_s"+i,All)!).ToArray();
  string State(object r){var m=rngField.GetValue(r)!;return string.Join(',',words.Select(f=>((ulong)f.GetValue(m)!).ToString("X16")));}
  var alphabet=(string)helper.GetField("_characters",All)!.GetRawConstantValue()!;
  var defaultLength=(int)helper.GetField("seedDefaultLength",All)!.GetRawConstantValue()!;
  var inputs=new List<string>{""," "," \t\r\n ","A","a","0","O","I","0123456789AB","nspire-i-o","  mixED-Case_123  ","oldA","olda","OLDa","old","!@#$%^&*()_+-=[]{};:'\",.<>/?\\|`~"};
  for(int c=32;c<=126;c++)inputs.Add(((char)c).ToString());
  foreach(int n in new[]{1,11,12,13,31,32,33,63,64,65,127,128,255,256,1024})inputs.Add(new string('a',n));
  foreach(char c in alphabet)for(int at=0;at<defaultLength;at++){var text=new string('0',defaultLength).ToCharArray();text[at]=c;inputs.Add(new string(text));}
  var originalGenerator=randomType.GetConstructor(new[]{typeof(ulong)})!.Invoke(new object[]{0x24a5c11UL});
  for(int i=0;i<128;i++)inputs.Add((string)generate.Invoke(null,new object[]{originalGenerator,defaultLength})!);
  var cases=new Random(0x24a5c11);for(int i=0;i<256;i++)inputs.Add(new string(Enumerable.Range(0,cases.Next(1,65)).Select(_=>(char)cases.Next(32,127)).ToArray()));
  var streams=new[]{"up_front","shuffle","unknown_map_point","combat_card_generation","combat_potion_generation","combat_card_selection","combat_energy_costs","combat_targets","monster_ai","niche","combat_orbs","treasure_room_relics","rewards","shops","transformations"};
  var regions=new[]{"Overgrowth","Underdocks","Hive","Glory"};
  var mapCtor=mapType.GetConstructors(All).Single(c=>c.GetParameters().Length==7);
  string Coord(object point){var c=point.GetType().GetField("coord",All)!.GetValue(point)!;return c.GetType().GetField("col",All)!.GetValue(c)+","+c.GetType().GetField("row",All)!.GetValue(c);}
  string Graph(object map){
   var points=((IEnumerable)mapType.GetMethod("GetAllMapPoints",All)!.Invoke(map,null)!).Cast<object>().ToList();
   foreach(string name in new[]{"StartingMapPoint","BossMapPoint","SecondBossMapPoint"}){var p=mapType.GetProperty(name,All)!.GetValue(map);if(p!=null&&!points.Contains(p))points.Add(p);}
   return string.Join(';',points.OrderBy(Coord,StringComparer.Ordinal).Select(p=>Coord(p)+","+Convert.ToInt32(p.GetType().GetProperty("PointType",All)!.GetValue(p))+">"+
    string.Join('|',((IEnumerable)p.GetType().GetProperty("Children",All)!.GetValue(p)!).Cast<object>().Select(Coord).OrderBy(x=>x,StringComparer.Ordinal))));
  }
  string path=Path.Combine(output,"ascii-original.tsv");int rngRows=0,mapRows=0;
  using(var file=new StreamWriter(path){NewLine="\n"})for(int i=0;i<inputs.Count;i++){
   string input=inputs[i],normal=canonical(input);
   var rawRun=runCtor.Invoke(new object[]{input});var run=runCtor.Invoke(new object[]{normal});
   ulong raw=(ulong)runType.GetProperty("Seed")!.GetValue(rawRun)!,seed=(ulong)runType.GetProperty("Seed")!.GetValue(run)!;
   file.WriteLine($"S\t{i}\t{Hex(input)}\t{Hex(normal)}\t{raw:X16}\t{seed:X16}");
   var dictionary=(IDictionary)runType.GetField("_rngs",All)!.GetValue(run)!;
   for(int stream=0;stream<streams.Length;stream++){
    object r=stream<12?dictionary[Enum.ToObject(assembly.GetType("MegaCrit.Sts2.Core.Entities.Rngs.RunRngType",true)!,stream)]!:namedCtor.Invoke(new object[]{seed,streams[stream]});
    string initial=State(r);var draws=new List<string>();for(int draw=0;draw<8;draw++)draws.Add(((ulong)next.Invoke(r,null)!).ToString("X16"));
    file.WriteLine($"R\t{i}\t{streams[stream]}\t{initial}\t{string.Join(',',draws)}\t{State(r)}\t{countField.GetValue(r)}");++rngRows;
   }
   // Same explicit solo region and ascension flags in both implementations.
   // This does not execute RunState/Timeline/encounter-pool initialization.
   if(Encoding.UTF8.GetByteCount(normal)>64)continue;
   for(int region=0;region<4;region++)for(int elite=0;elite<2;elite++){
    var r=namedCtor.Invoke(new object[]{seed,"act_"+(region<2?1:region)+"_map"});
    var act=RuntimeHelpers.GetUninitializedObject(assembly.GetType("MegaCrit.Sts2.Core.Models.Acts."+regions[region],true)!);
    var counts=act.GetType().GetMethod("GetMapPointTypes",All)!.Invoke(act,new[]{r})!;
    counts.GetType().GetProperty("NumOfElites",All)!.SetValue(counts,elite==0?5:8);
    string quota=string.Join(',',new[]{"NumOfRests","NumOfShops","NumOfElites","NumOfUnknowns"}.Select(n=>counts.GetType().GetProperty(n,All)!.GetValue(counts)));
    int quotaCalls=(int)countField.GetValue(r)!;
    bool treasure=i%7==0,second=region==3&&i%3==0;
    var map=mapCtor.Invoke(new object?[]{r,act,false,treasure,second,counts,true});
    file.WriteLine($"M\t{i}\t{region}\t{elite}\t{(treasure?1:0)}\t{(second?1:0)}\t{quota}\t{quotaCalls}\t{countField.GetValue(r)}\t{State(r)}\t{Graph(map)}");++mapRows;
   }
  }
  var receipt=new{reference="v0.111.0 / 41cef1ea",runtime=RuntimeInformation.FrameworkDescription,os=RuntimeInformation.OSDescription,
   culture=CultureInfo.CurrentCulture.Name,alphabet,default_seed_length=defaultLength,seed_cases=inputs.Count,rng_stream_cases=rngRows,map_cases=mapRows,
   actual_generated_seeds=128,generated_alphabet_positions=alphabet.Length*defaultLength,printable_ascii_singletons=95,random_printable_strings=256,
   original_methods=new[]{"CanonicalizeSeed","RunRngSet(string)","Rng(ulong,string)","Rng.NextUnsignedLong","GetMapPointTypes","StandardActMap constructor"},
   notes="12 RunRngSet streams plus 3 direct named Rng constructors; explicit solo StandardActMap configurations, not whole-run or Windows-runtime proof",
   output_sha256=Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path))).ToLowerInvariant()};
  File.WriteAllText(Path.Combine(output,"ascii-receipt.json"),JsonSerializer.Serialize(receipt,new JsonSerializerOptions{WriteIndented=true})+"\n");
  Console.WriteLine(JsonSerializer.Serialize(receipt));
 }
}
