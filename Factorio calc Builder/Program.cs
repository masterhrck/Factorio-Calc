using System;
using System.IO;
using System.Security.Cryptography;
using System.Collections.Generic;
using System.Xml.Serialization;
using System.Linq;
using System.Diagnostics;

namespace Factorio_calc_Builder
{
	public class HashEntry
	{
		public string filename;
		public string hash;
	}
	class Program
	{

		static void Main(string[] args)
		{
			string targetDir = @"C:\Users\Masterhrck\source\repos\Factorio calc\Factorio calc";
			string targetExe = @"C:\Users\Masterhrck\source\repos\Factorio calc\Factorio calc\Release\Factorio calc.exe";
			string saveFilename = "hashes.xml";
			string buildCommandFile = "cmd";
			string buildCommandArg = @"/C """"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.com"" ""C:\Users\Masterhrck\source\repos\Factorio calc\Factorio solution.sln"" /build Release /project ""C:\Users\Masterhrck\source\repos\Factorio calc\Factorio calc\Factorio calc.vcxproj"" /projectconfig Release""";
			XmlSerializer ser = new XmlSerializer(typeof(List<HashEntry>));
			List<string> filenames = new List<string>();

			{
				DirectoryInfo dir = new DirectoryInfo(targetDir);
				IEnumerable<FileInfo> files = dir.EnumerateFiles();
				foreach (FileInfo file in files)
				{
					filenames.Add(file.FullName);
				}
			}

			//Generate new hash entries
			Console.WriteLine("Generating hashes..");
			MD5 md5 = MD5.Create();
			List<HashEntry> newHashEntries = new List<HashEntry>();
			foreach (string filename in filenames)
			{
				string hashString;
				using (FileStream stream = File.OpenRead(filename))
				{
					byte[] hashBytes = md5.ComputeHash(stream);
					hashString = BitConverter.ToString(hashBytes).Replace("-", "").ToLowerInvariant();
				}
				newHashEntries.Add(new HashEntry { filename = filename, hash = hashString });
			}

			//Read old hash entries from file
			List<HashEntry> oldHashEntries = new List<HashEntry>();
			if (File.Exists(saveFilename))
			{
				Console.WriteLine("Reading old hashes..\n");
				using (TextReader reader = new StreamReader(saveFilename))
				{
					oldHashEntries = (List<HashEntry>)ser.Deserialize(reader);
				}
			}

			//Compare old and new
			var query = from newHashes in newHashEntries
						join oldHashes in oldHashEntries on newHashes.filename equals oldHashes.filename into oldHashes
						from items in oldHashes.DefaultIfEmpty(new HashEntry { hash = String.Empty })
						select new { Filename = newHashes.filename, OldHash = items.hash, NewHash = newHashes.hash };
			var results = query.ToList();
			bool ok = true;
			foreach (var result in results)
			{
				bool comp = result.OldHash == result.NewHash;
				if (comp==false)
					ok = false;
				Console.WriteLine($"{(comp ? "OK" : "DIFF ")} {result.OldHash} {result.NewHash} {Path.GetFileName(result.Filename)}");
			}
			Console.WriteLine();

			if (!ok)
			{
				//Start rebuilding by calling vs
				Console.WriteLine("Rebuilding executable..");
				using (Process p = new Process())
				{
					p.StartInfo.UseShellExecute = false;
					p.StartInfo.RedirectStandardOutput = true;
					p.StartInfo.FileName = buildCommandFile;
					p.StartInfo.Arguments = buildCommandArg;
					p.Start();
					while (true)
					{
						string output = p.StandardOutput.ReadLine();
						if (output == null)
							break;
						else
							Console.WriteLine(output);
					}
					p.WaitForExit();
				}
				Console.WriteLine("Build job complete");
			}
			else
			{
				Console.WriteLine("No differences found");
			}

			//Launching factorio calc
			Console.WriteLine("\nStarting Factorio calc");
			using (Process p = new Process())
			{
				p.StartInfo.UseShellExecute = true;
				p.StartInfo.CreateNoWindow = false;
				p.StartInfo.WindowStyle = ProcessWindowStyle.Normal;
				p.StartInfo.FileName = targetExe;
				p.StartInfo.WorkingDirectory = Path.GetDirectoryName(targetExe);
				p.Start();
			}

			if (!ok)
			{
				//Write new hash entries to file
				Console.WriteLine("Writing new hashes to file..");
				using (TextWriter writer = new StreamWriter(saveFilename))
				{
					ser.Serialize(writer, newHashEntries);
				}
				Console.WriteLine("\nPress any key to exit..");
				Console.ReadKey();
			}
		}
	}
}
