#define uwp
//#define uwp_build

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO;
using System.Text;
using System;
using System.Threading;
using System.Linq;



#if uwp
#if !uwp_build
using System.Windows.Networking;
using System.Windows.Networking.Sockets;
using UnityEngine.Networking;
using System.Windows.Foundation;
#endif
#else
using System.Net;
using System.Net.Sockets;
#endif
namespace NetDev_Client
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Hello World!");
        }
    }
}
