#include  <WbemIdl.h>
#pragma comment(lib, "wbemuuid")
#include  <string>
#include  <vector>

#define SAFE_RELEASE(ptr) if(ptr){(ptr)->Release(); (ptr) = nullptr;}

// class: NetworkAddress
class NetworkAddress
{
public:
  int SetAddress(const std::wstring &ip_address, const std::wstring &subnet_mask)
  {
    m_ip_address = ip_address;
    m_subnet_mask = subnet_mask;
    return 0;
  }
  const std::wstring& GetIPAddress() const
  {
    return m_ip_address;
  }
  const std::wstring& GetSubnetMask() const
  {
    return m_subnet_mask;
  }
private:
  std::wstring m_ip_address;
  std::wstring m_subnet_mask;
};

// target_network_adaptorで指定しているアダプター名にnetwork_addressを追加する。
long FirstFindAdaptorAddStaticIPAddress(const std::wstring &target_network_adaptor, const std::vector<NetworkAddress> &network_address);

// application entry point.
int main()
{
  // WMIのCOMクライアントはMTAでなければならない。
  HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if(SUCCEEDED(hr))
  {
    // 割り当てるIPの設定
    std::vector<NetworkAddress> network_addres;
    {
      NetworkAddress  address1;
      address1.SetAddress(L"192.168.0.100", L"255.255.0.0");
      network_addres.push_back(address1);
    }
    {
      NetworkAddress  address2;
      address2.SetAddress(L"192.168.0.101", L"255.255.0.0");
      network_addres.push_back(address2);
    }
    const std::wstring &target_adaptor = L"PCI";  // ネットワークアダプター名にPCIがつくアダプターを対象（最初に見つかったもの)
    const int result = FirstFindAdaptorAddStaticIPAddress(target_adaptor, network_addres);
    ::CoUninitialize();
    return result;
  }
  else
  {
    return 1;
  }
}
// target_network_adaptorで指定しているアダプター名にnetwork_addressを追加する。
long FirstFindAdaptorAddStaticIPAddress(const std::wstring &target_network_adaptor, const std::vector<NetworkAddress> &network_addres)
{
  IWbemLocator *locator = nullptr;
  HRESULT hr = ::CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&locator));
  /* CIMV2名前空間 */
  IWbemServices *targetNamespace = nullptr;
  if(SUCCEEDED(hr))
  {
    BSTR namespacePath = ::SysAllocString(L"root\\cimv2");
    hr = locator->ConnectServer(namespacePath, nullptr, nullptr,
                                nullptr, 0, nullptr, 
                                nullptr, &targetNamespace);
    ::SysFreeString(namespacePath);
  }

  /* セキュリティ設定 */
  if(SUCCEEDED(hr))
  {
    hr = ::CoSetProxyBlanket(targetNamespace, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                             nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                             nullptr, EOAC_NONE);
  }

  /* クラスオブジェクト */
  IWbemClassObject *wmi_class = nullptr;
  BSTR wmi_class_name = ::SysAllocString(L"Win32_NetworkAdapterConfiguration");
  if(SUCCEEDED(hr))
  {
    hr = targetNamespace->GetObjectW(wmi_class_name, 0, nullptr, &wmi_class, nullptr);
  }

  /* メソッド */
  IWbemClassObject *wmi_method = nullptr;
  BSTR method_name = ::SysAllocString(L"EnableStatic");
  if(SUCCEEDED(hr))
  {
    hr = wmi_class->GetMethod(method_name, 0, &wmi_method, nullptr);
  }

  /* ネットワークアドレスのパラメーター */
  IWbemClassObject *network_address_param = nullptr;
  if(SUCCEEDED(hr))
  {
    hr = wmi_method->SpawnInstance(0, &network_address_param);
  }

  /* パラメーターにIPアドレス設定 */
  if(SUCCEEDED(hr))
  {
    VARIANT address_var;
    VARIANT subnet_mask_var;
    {
      SAFEARRAYBOUND sab[1] = {0};
      sab[0].cElements  = network_addres.size();
      // IPアドレス
      address_var.vt          = VT_ARRAY | VT_BSTR;
      address_var.parray      = ::SafeArrayCreate(VT_BSTR, 1, sab);
      // サブネットマスク
      subnet_mask_var.vt      = VT_ARRAY | VT_BSTR;
      subnet_mask_var.parray  = ::SafeArrayCreate(VT_BSTR, 1, sab);
      // 配列に設定
      LONG index = 0;
      for(auto iter = std::begin(network_addres); iter != std::end(network_addres); ++iter)
      {
        if(SUCCEEDED(hr))
        {
          BSTR ip_address = ::SysAllocString(iter->GetIPAddress().c_str());
          hr = ::SafeArrayPutElement(address_var.parray, &index, ip_address);
          ::SysFreeString(ip_address);
        }

        if(SUCCEEDED(hr))
        {
          BSTR subnet_mask = ::SysAllocString(iter->GetSubnetMask().c_str());
          hr = ::SafeArrayPutElement(subnet_mask_var.parray, &index, subnet_mask);
          ::SysFreeString(subnet_mask);
        }

        ++index;
      }
    }
    if(SUCCEEDED(hr))
    {
      BSTR param_ip_address   = ::SysAllocString(L"IPAddress");
      hr = network_address_param->Put(param_ip_address, 0, &address_var,
                                      0);
      ::SysFreeString(param_ip_address);
    }
    if(SUCCEEDED(hr))
    {
      BSTR param_subnet_mask  = ::SysAllocString(L"SubnetMask");
      network_address_param->Put(param_subnet_mask, 0, &subnet_mask_var,
                                 0);
      ::SysFreeString(param_subnet_mask);
    }

    ::VariantClear(&subnet_mask_var);
    ::VariantClear(&address_var);
  }
  
  /* ネットワークアダプターを探す */
  IEnumWbemClassObject *adapter_enumerator = nullptr;
  if(SUCCEEDED(hr))
  {
    hr = targetNamespace->CreateInstanceEnum(wmi_class_name, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
                                             &adapter_enumerator);
  }

  /* ネットワークアダプター インスタンスへのパスを取得 */
  BSTR network_adaptr_instance;
  if(SUCCEEDED(hr))
  {
    // 対象のネットワークアダプターのインスタンス取得
    IWbemClassObject *instance = nullptr;
    do
    {
      ULONG ret = 0u;
      hr = adapter_enumerator->Next(WBEM_INFINITE, (ULONG)1, &instance, &ret);
      if(SUCCEEDED(hr) && (ret != 0))
      {
        BSTR desc_property = ::SysAllocString(L"Description");
        VARIANT desc;
        hr = instance->Get(desc_property, 0, &desc, nullptr, nullptr);
        ::SysFreeString(desc_property);
        if(SUCCEEDED(hr))
        {
          const std::wstring &adaptor_name = desc.bstrVal;
          ::VariantClear(&desc);
          auto b_find = adaptor_name.find(target_network_adaptor);
          if(b_find != std::wstring::npos)
          {
            break;  // do_whileをぬける
          }

        }

      }
      else
      {
        break;  // do_whileをぬける
      }
    }
    while(hr != WBEM_S_FALSE);
    // インスタンスへのパス取得
    if(SUCCEEDED(hr))
    {
      BSTR path_property = ::SysAllocString(L"__PATH");
      VARIANT path_var;
      hr = instance->Get(path_property, 0, &path_var, nullptr, nullptr);
      ::SysFreeString(path_property);
      if(SUCCEEDED(hr))
      {
        network_adaptr_instance = path_var.bstrVal;
      }

      ::VariantClear(&path_var);
    }

    SAFE_RELEASE(instance);
  }

  /* IPアドレスの設定 */
  IWbemClassObject *result_class = nullptr;
  if(SUCCEEDED(hr))
  {
    hr = targetNamespace->ExecMethod(network_adaptr_instance, method_name, 0, nullptr, network_address_param, &result_class, nullptr);
  }
  
  long result = -1;
  if(FAILED(hr))
  {
    result = 1;
  }
  else
  {
    BSTR return_value = SysAllocString(L"ReturnValue");
    VARIANT res_var;
    hr = result_class->Get(return_value, 0, &res_var, 0, 0);
    result = V_I4(&res_var);
    ::SysFreeString(return_value);
    ::VariantClear(&res_var);
  }
  SAFE_RELEASE(result_class);
  ::SysFreeString(network_adaptr_instance);
  SAFE_RELEASE(adapter_enumerator);
  SAFE_RELEASE(network_address_param);
  SAFE_RELEASE(wmi_method);
  ::SysFreeString(method_name);
  SAFE_RELEASE(wmi_class);
  ::SysFreeString(wmi_class_name);
  SAFE_RELEASE(targetNamespace);
  SAFE_RELEASE(locator);
  return result;
}
