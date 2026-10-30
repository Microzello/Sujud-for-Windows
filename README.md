# Sujud for Windows

Prayer times for where you are, and nothing else. Native C++ and WinUI 3 for Windows 11, with widgets for the Widgets board and the lock screen, an Xbox Game Bar widget, and an MCP server for AI agents on Windows.

## Requirements

- Windows 11 with Developer Mode on
- Visual Studio 2026 with "Desktop development with C++" and "WinUI application development", including "C++ WinUI app development tools", plus "C++ Universal Windows Platform tools" for the Game Bar widget
- Node.js, only to regenerate the reference test data

## Run

Open `Sujud.slnx`, set `Sujud` as the startup project, pick x64 or ARM64, and press F5.

## Develop

Engine tests and benchmark, from a Developer PowerShell:

```powershell
msbuild tests\Core.Tests\Sujud.Core.Tests.vcxproj -restore -p:Configuration=Release -p:Platform=x64
bin\x64\Release\Sujud.Core.Tests\Sujud.Core.Tests.exe
bin\x64\Release\Sujud.Core.Tests\Sujud.Core.Tests.exe --bench
```

Call the MCP server directly, after deploying the app:

```powershell
'{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_next_prayer","arguments":{"latitude":21.4225,"longitude":39.8262}}}' | Sujud.Mcp.exe
```

Regenerate the praytime.js reference cases:

```powershell
cd tests\reference
npm install
npm run generate
```

## License

MIT, see [LICENSE](LICENSE). The prayer time calculations are rewritten from [praytime.js](https://praytimes.org), also MIT, see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
