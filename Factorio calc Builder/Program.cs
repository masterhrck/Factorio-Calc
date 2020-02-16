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
            string[] filenames = {
                @"C:\Users\Masterhrck\Desktop\hashme.txt",
                @"C:\Users\Masterhrck\Desktop\hashme2.txt",
            };
            string saveFilename = "hashes.xml";
            string buildCommandFile = @"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe";
            string buildCommandArg = @"""C:\Users\Masterhrck\source\repos\Factorio calc\Factorio solution.sln"" /rebuild Release /project ""C:\Users\Masterhrck\source\repos\Factorio calc\Factorio calc\Factorio calc.vcxproj"" /projectconfig Release";

            XmlSerializer ser = new XmlSerializer(typeof(List<HashEntry>));

            //Generate new hash entries
            MD5 md5 = MD5.Create();
            List<HashEntry> newHashEntries = new List<HashEntry>();
            foreach (string filename in filenames)
            {
                string hashString;
                using (var stream = File.OpenRead(filename))
                {
                    byte[] hashByte = md5.ComputeHash(stream);
                    hashString = BitConverter.ToString(hashByte).Replace("-", "").ToLowerInvariant();
                }
                newHashEntries.Add(new HashEntry { filename = filename, hash = hashString });
            }
            
            //Read old hash entries from file
            List<HashEntry> oldHashEntries = new List<HashEntry>();
            using (TextReader reader = new StreamReader(saveFilename))
            {
                oldHashEntries = (List<HashEntry>)ser.Deserialize(reader);
            }

            var query = from newHashes in newHashEntries
                         join oldHashes in oldHashEntries on newHashes.filename equals oldHashes.filename into oldHashes
                         from items in oldHashes.DefaultIfEmpty(new HashEntry { hash=String.Empty})
                         select new { Filename = newHashes.filename, OldHash = items.hash, NewHash = newHashes.hash };
            var results = query.ToList();
            bool ok = true;
            foreach(var result in results)
            {
                if (result.OldHash != result.NewHash)
                    ok = false;
                Console.WriteLine($"{(ok ? "OK" : "DIFF ")} {result.OldHash} {result.NewHash} {result.Filename}");
            }
            Console.WriteLine();
            if (!ok)
            {
                using(Process process = new Process()){
                    //TODO Feedback from builder, exception handling
                    Console.WriteLine("Rebuilding executable..");
                    process.StartInfo.UseShellExecute = false;
                    process.StartInfo.FileName = buildCommandFile;
                    process.StartInfo.Arguments = buildCommandArg;

                    process.Start();
                    process.WaitForExit();
                    Console.WriteLine("Done");
                }
            }

            using (Process process = new Process())
            {
                //TODO Start factorio calc
            }

            //Write new hash entries to file
            using (TextWriter writer = new StreamWriter(saveFilename))
            {
                ser.Serialize(writer, newHashEntries);
            }




            Console.WriteLine("Press any key to exit..");
            Console.ReadKey();
        }
    }
}
