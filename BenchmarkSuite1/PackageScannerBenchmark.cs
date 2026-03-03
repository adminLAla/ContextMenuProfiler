using BenchmarkDotNet.Attributes;
using ContextMenuProfiler.UI.Core;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VSDiagnostics;

[CPUUsageDiagnoser]
public class PackageScannerBenchmark
{
    [Benchmark]
    public List<BenchmarkResult> ScanPackagedExtensions()
    {
        return PackageScanner.ScanPackagedExtensions(null).ToList();
    }
}