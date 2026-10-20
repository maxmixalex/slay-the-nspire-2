// Independent offline adapter. Supply the separately licensed Spine 4.2 runtime
// locally; neither that runtime nor game assets belong in this source repository.
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text.Json;
using Spine;

sealed class AtlasDimensions : TextureLoader {
    public void Load(AtlasPage page, string path) {
        if (page.width <= 0 || page.height <= 0)
            throw new InvalidDataException("Atlas page must declare its dimensions: " + path);
        page.rendererObject = path;
    }
    public void Unload(object texture) { }
}

static class SpineSampleAdapter {
    public const string RuntimeSha256 = "0e05031a0377bea228e6419572f171496cb77d58bfdc9129c4607d54601ba3ec";
    static readonly Dictionary<string, SkeletonData> Cache = new();
    static readonly int[] QuadTriangles = { 0, 1, 2, 2, 3, 0 };
    static string String(JsonElement j, string k, string fallback = "") => j.TryGetProperty(k, out var v) ? v.GetString() ?? fallback : fallback;
    static int Int(JsonElement j, string k, int fallback = 0) => j.TryGetProperty(k, out var v) ? v.GetInt32() : fallback;
    static float Float(JsonElement j, string k, float fallback = 0) => j.TryGetProperty(k, out var v) ? v.GetSingle() : fallback;
    static bool Bool(JsonElement j, string k, bool fallback = false) => j.TryGetProperty(k, out var v) ? v.GetBoolean() : fallback;
    static T[] Copy<T>(ExposedList<T> list) => list.Items.Take(list.Count).ToArray();

    static SkeletonData Load(JsonElement request) {
        string skel = Path.GetFullPath(String(request, "skeleton_path"));
        string atlas = Path.GetFullPath(String(request, "atlas_path"));
        string key = skel + "\n" + atlas;
        if (!Cache.TryGetValue(key, out var data)) {
            using (var stream = File.OpenRead(skel)) {
                string version = SkeletonBinary.GetVersionString(stream);
                if (!version.StartsWith("4.2.", StringComparison.Ordinal))
                    throw new InvalidDataException("Expected Spine 4.2 binary, found " + version);
            }
            var regions = new Atlas(atlas, new AtlasDimensions());
            data = new SkeletonBinary(regions).ReadSkeletonData(skel);
            if (data == null || !data.Version.StartsWith("4.2.", StringComparison.Ordinal))
                throw new InvalidDataException("Spine runtime could not read the expected 4.2 skeleton");
            Cache.Add(key, data);
        }
        return data;
    }

    // Preserve numeric constraint facts without copying the original binary or
    // runtime implementation. Object links are represented only by their names.
    static object Facts(object value) {
        if (value == null) return null;
        var result = new SortedDictionary<string, object>(StringComparer.Ordinal);
        foreach (var prop in value.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance)) {
            if (prop.GetIndexParameters().Length != 0 || !prop.CanRead) continue;
            var item = prop.GetValue(value);
            if (item == null) continue;
            var kind = item.GetType();
            if (kind.IsPrimitive || item is string || item is decimal) result[prop.Name] = item;
            else if (kind.IsEnum) result[prop.Name] = item.ToString();
            else if (item is IEnumerable sequence) {
                var names = new List<string>();
                foreach (var child in sequence) {
                    var name = child?.GetType().GetProperty("Name")?.GetValue(child)?.ToString();
                    if (name != null) names.Add(name);
                }
                result[prop.Name] = names;
            } else {
                var name = kind.GetProperty("Name")?.GetValue(item)?.ToString();
                if (name != null) result[prop.Name] = name;
            }
        }
        return result;
    }

    static object Metadata(SkeletonData data) {
        return new {
            source_version = data.Version,
            skeleton_hash = data.Hash,
            available_skins = data.Skins.Select(s => s.Name).ToArray(),
            skin_details = data.Skins.Select(s => new {
                name = s.Name, attachments = s.Attachments.Count,
                bones = s.Bones.Select(b => b.Name).ToArray(),
                constraints = s.Constraints.Select(c => c.Name).ToArray()
            }).ToArray(),
            bone_count = data.Bones.Count, slot_count = data.Slots.Count,
            two_color_slots = data.Slots.Where(s => s.HasSecondColor).Select(s => s.Name).ToArray(),
            clips = data.Animations.Select(a => new {
                name = a.Name, duration = a.Duration,
                duration_ms = Math.Max(1, (int)Math.Round(a.Duration * 1000.0)),
                events = a.Timelines.OfType<EventTimeline>().SelectMany(t => t.Events)
                    .Select(e => new { time = e.Time, name = e.Data.Name }).ToArray(),
                timeline_types = a.Timelines.GroupBy(t => t.GetType().Name)
                    .ToDictionary(g => g.Key, g => g.Count(), StringComparer.Ordinal)
            }).ToArray(),
            constraints = new {
                ik = data.IkConstraints.Select(c => Facts(c)).ToArray(),
                transform = data.TransformConstraints.Select(c => Facts(c)).ToArray(),
                path = data.PathConstraints.Select(c => Facts(c)).ToArray(),
                physics = data.PhysicsConstraints.Select(c => Facts(c)).ToArray()
            }
        };
    }

    static float[] Matrix(Bone bone) => new[] { bone.A, bone.B, bone.WorldX, -bone.C, -bone.D, -bone.WorldY };

    static float[] NodeMatrix(Bone bone) {
        // Godot's Follow nodes rebuild their matrix from world rotation and
        // axis lengths. Their transform intentionally carries no Spine shear.
        float radians = bone.WorldRotationX * (MathF.PI / 180f);
        float cos = MathF.Cos(radians), sin = MathF.Sin(radians);
        return new[] { bone.WorldScaleX * cos, -bone.WorldScaleX * sin, bone.WorldX,
                       bone.WorldScaleY * sin, bone.WorldScaleY * cos, bone.WorldY };
    }
    static float[][] NodeMatrices(Skeleton skeleton) => skeleton.Bones.Select(NodeMatrix).ToArray();

    static object Frame(Skeleton skeleton, float time, float[][] followers) {
        var draw = new List<object>();
        var slots = new List<object>();
        var clipper = new SkeletonClipping();
        foreach (var slot in skeleton.DrawOrder) {
            slots.Add(new {
                slot = slot.Data.Name, index = slot.Data.Index, bone = slot.Bone.Data.Name,
                matrix = Matrix(slot.Bone), node_matrix = followers[slot.Bone.Data.Index], active = slot.Bone.Active,
                attachment = slot.Attachment?.Name
            });
            if (!slot.Bone.Active) { clipper.ClipEnd(slot); continue; }
            if (slot.Attachment is ClippingAttachment clipping) {
                clipper.ClipStart(slot, clipping);
                continue;
            }
            float[] vertices, uvs;
            int[] triangles;
            AtlasRegion region;
            float red, green, blue, alpha;
            if (slot.Attachment is RegionAttachment quad) {
                vertices = new float[8];
                quad.ComputeWorldVertices(slot, vertices, 0);
                uvs = quad.UVs; triangles = QuadTriangles;
                region = (AtlasRegion)quad.Region;
                red = quad.R; green = quad.G; blue = quad.B; alpha = quad.A;
            } else if (slot.Attachment is MeshAttachment mesh) {
                vertices = new float[mesh.WorldVerticesLength];
                mesh.ComputeWorldVertices(slot, 0, vertices.Length, vertices, 0);
                uvs = mesh.UVs; triangles = mesh.Triangles;
                region = (AtlasRegion)mesh.Region;
                red = mesh.R; green = mesh.G; blue = mesh.B; alpha = mesh.A;
            } else { clipper.ClipEnd(slot); continue; }
            if (clipper.IsClipping) {
                clipper.ClipTriangles(vertices, triangles, triangles.Length, uvs);
                vertices = Copy(clipper.ClippedVertices);
                triangles = Copy(clipper.ClippedTriangles);
                uvs = Copy(clipper.ClippedUVs);
            }
            for (int coordinate = 1; coordinate < vertices.Length; coordinate += 2) vertices[coordinate] = -vertices[coordinate];
            if (triangles.Length != 0 && skeleton.A * slot.A * alpha > 0) {
                if (vertices.Any(v => !float.IsFinite(v)) || uvs.Any(v => !float.IsFinite(v)))
                    throw new InvalidDataException("Non-finite world geometry in " + slot.Data.Name);
                draw.Add(new {
                    slot = slot.Data.Name, index = slot.Data.Index,
                    attachment = slot.Attachment.Name, region = region.name, page = region.page.name,
                    premultiplied_alpha = region.page.pma,
                    vertices, uvs, triangles,
                    tint = new[] { skeleton.R * slot.R * red, skeleton.G * slot.G * green,
                                   skeleton.B * slot.B * blue, skeleton.A * slot.A * alpha },
                    dark = slot.HasSecondColor ? new[] { slot.R2, slot.G2, slot.B2 } : null,
                    blend = (int)slot.Data.BlendMode
                });
            }
            clipper.ClipEnd(slot);
        }
        clipper.ClipEnd();
        return new { time, draw, slots,
            draw_order = skeleton.DrawOrder.Select(slot => slot.Data.Name).ToArray(),
            bones = skeleton.Bones.Select(b => new {
            name = b.Data.Name, matrix = Matrix(b), node_matrix = followers[b.Data.Index], active = b.Active
        }).ToArray() };
    }

    static void SetTrack(AnimationState state, JsonElement track) {
        int index = Int(track, "track");
        if (index < 0 || index > 15) throw new InvalidDataException("Track index out of bounds");
        TrackEntry entry;
        if (String(track, "clip") == "") {
            if (Bool(track, "queued")) entry = state.AddEmptyAnimation(index, Float(track, "mix"), Float(track, "delay"));
            else entry = state.SetEmptyAnimation(index, Float(track, "mix"));
        } else if (Bool(track, "queued"))
            entry = state.AddAnimation(index, String(track, "clip"), Bool(track, "loop"), Float(track, "delay"));
        else entry = state.SetAnimation(index, String(track, "clip"), Bool(track, "loop"));
        if (track.TryGetProperty("time", out var offset)) {
            float time = offset.GetSingle();
            if (!float.IsFinite(time) || time < 0 || time > 120)
                throw new InvalidDataException("Track pose offset must be finite within [0, 120]");
            entry.TrackTime = time;
        }
    }

    // A Drive node supplies a transform relative to its SpineSprite. This is
    // Godot's canvas convention (y down); emitted geometry remains y up.
    static float[] ReadDriveMatrix(JsonElement value) {
        var matrix = value.EnumerateArray().Select(v => v.GetSingle()).ToArray();
        if (matrix.Length != 6 || matrix.Any(v => !float.IsFinite(v)))
            throw new InvalidDataException("Driven bone matrix must have six finite components");
        return matrix;
    }
    static float[] DriveMatrix(JsonElement drive, float time) {
        var current = ReadDriveMatrix(drive.GetProperty("matrix"));
        if (!drive.TryGetProperty("keyframes", out var keys)) return current;
        float previousTime = 0;
        foreach (var key in keys.EnumerateArray()) {
            float nextTime = Float(key, "time");
            if (!float.IsFinite(nextTime) || nextTime < previousTime)
                throw new InvalidDataException("Driven bone keyframes must have ordered finite times");
            var next = ReadDriveMatrix(key.GetProperty("matrix"));
            if (time < nextTime) {
                float weight = Bool(drive, "hold") || nextTime == previousTime ? 0 : Math.Clamp((time - previousTime) / (nextTime - previousTime), 0, 1);
                for (int i = 0; i < 6; ++i) current[i] += (next[i] - current[i]) * weight;
                return current;
            }
            current = next; previousTime = nextTime;
        }
        return current;
    }
    static void DriveBones(Skeleton skeleton, JsonElement request, float time) {
        if (!request.TryGetProperty("bone_drives", out var drives)) return;
        bool changed = false;
        foreach (var drive in drives.EnumerateArray()) {
            if (!Bool(drive, "enabled", true)) continue;
            var bone = skeleton.FindBone(String(drive, "bone"));
            if (bone == null) throw new InvalidDataException("Unknown driven bone: " + String(drive, "bone"));
            float[] m = DriveMatrix(drive, time);
            float x = m[2], y = m[5];
            if (bone.Parent != null) bone.Parent.WorldToLocal(x, y, out x, out y);
            float rotation = MathF.Atan2(m[3], m[0]) * (180f / MathF.PI);
            float sx = MathF.Sqrt(m[0] * m[0] + m[3] * m[3]);
            float sy = MathF.CopySign(MathF.Sqrt(m[1] * m[1] + m[4] * m[4]), m[0] * m[4] - m[1] * m[3]);
            bone.X = x; bone.Y = y;
            bone.Rotation = bone.WorldToLocalRotation(rotation) - 180f;
            bone.ScaleX = sx; bone.ScaleY = sy;
            changed = true;
        }
        if (changed) skeleton.UpdateWorldTransform(Skeleton.Physics.Update);
    }

    static object Samples(SkeletonData data, JsonElement request) {
        var skeleton = new Skeleton(data);
        if (request.TryGetProperty("skins", out var selected)) {
            var skin = new Skin("offline-composite");
            foreach (var name in selected.EnumerateArray()) {
                var source = data.FindSkin(name.GetString());
                if (source == null) throw new InvalidDataException("Unknown skin: " + name.GetString());
                skin.AddSkin(source);
            }
            skeleton.SetSkin(skin);
        }
        skeleton.SetToSetupPose();
        var state = new AnimationState(new AnimationStateData(data));
        if (request.TryGetProperty("setup_tracks", out var setup))
            foreach (var track in setup.EnumerateArray()) SetTrack(state, track);
        if (String(request, "clip") != "")
            state.SetAnimation(Int(request, "track"), String(request, "clip"), Bool(request, "loop"));
        if (request.TryGetProperty("tracks", out var tracks))
            foreach (var track in tracks.EnumerateArray()) SetTrack(state, track);
        state.Apply(skeleton);
        skeleton.UpdateWorldTransform(Skeleton.Physics.Reset);
        var followers = NodeMatrices(skeleton);
        DriveBones(skeleton, request, 0);
        float time = 0;
        float step = Float(request, "physics_step", 1f / 60f);
        if (!(step > 0 && step <= 0.1f)) throw new InvalidDataException("physics_step must be positive and <= 0.1");
        var frames = new List<object>();
        foreach (var item in request.GetProperty("times").EnumerateArray()) {
            float target = item.GetSingle();
            if (!float.IsFinite(target) || target < time || target > 120 || frames.Count >= 256)
                throw new InvalidDataException("Sample times must be ordered within [0, 120], at most 256 frames");
            while (target - time > 0.000001f) {
                float delta = Math.Min(step, target - time);
                skeleton.Update(delta); state.Update(delta); state.Apply(skeleton);
                skeleton.UpdateWorldTransform(Skeleton.Physics.Update);
                followers = NodeMatrices(skeleton);
                time += delta; DriveBones(skeleton, request, time);
            }
            frames.Add(Frame(skeleton, target, followers));
        }
        return new { source_version = data.Version, physics_step = step, frames };
    }

    public static int Main(string[] args) {
        // Match the original Godot runtime; Frame converts back to y up.
        Bone.yDown = true;
        string path = typeof(Skeleton).Assembly.Location;
        string digest = Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path))).ToLowerInvariant();
        if (digest != RuntimeSha256) {
            Console.Error.WriteLine("Spine runtime SHA-256 differs from the reviewed 4.2 build: " + digest);
            return 2;
        }
        string line;
        while ((line = Console.ReadLine()) != null) {
            try {
                using var doc = JsonDocument.Parse(line);
                var request = doc.RootElement;
                if (Int(request, "schema_version", 1) != 1) throw new InvalidDataException("Unsupported adapter protocol");
                var data = Load(request);
                object result = String(request, "op") switch {
                    "metadata" => Metadata(data),
                    "samples" => Samples(data, request),
                    _ => throw new InvalidDataException("op must be metadata or samples")
                };
                Console.WriteLine(JsonSerializer.Serialize(new { ok = true, result }));
            } catch (Exception error) {
                Console.WriteLine(JsonSerializer.Serialize(new { ok = false, error = error.Message, type = error.GetType().Name }));
            }
        }
        return 0;
    }
}
