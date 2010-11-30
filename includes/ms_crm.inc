<?php  

/*php Microsoft Dynamics Crm 4.0 IFD authentication library

Copyright (c) 2009 Zenithies, Chuck Vose

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


class MSCrmIFD {
  public $usr;
  public $pwd;
  public $org;
  public $domain;

  public $crmHost;
  public $crmProto;
  public $crmPath;
  private $crmTicket;
  private $cURLHandle;

  // performs login
  public function getAccess() {
    $matches = array();

    // prepare request
    $request = '<?xml version="1.0" encoding="utf-8"?>
    <soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
    <soap:Body>
      <Execute xmlns="http://schemas.microsoft.com/crm/2007/CrmDiscoveryService">
        <Request xsi:type="RetrieveCrmTicketRequest">
          <OrganizationName>' . $this->org . '</OrganizationName>
          <UserId>' . $this->domain . '\\' . $this->usr .'</UserId>
          <Password>' . $this->pwd . '</Password>
        </Request>
      </Execute>
    </soap:Body>
    </soap:Envelope>';
    
    // TODO: Check that the path has a / at the front
    
    // setup headers 
    $headers = array(
      "POST " . $this->crmPath . " HTTP/1.1",
      "Host: " . $this->crmHost,
      'Connection: Keep-Alive',
      "SOAPAction: \"http://schemas.microsoft.com/crm/2007/CrmDiscoveryService/Execute\"",
      "Content-type: text/xml;charset=\"utf-8\"",
      "Content-length: " . strlen($request),
    );

    // Initialize cURL, this instance will be used for passing all requests to CrmService
    $this->cURLHandle = curl_init();

    curl_setopt($this->cURLHandle, CURLOPT_URL, $this->crmProto . '://' . $this->crmHost . $this->crmPath);
    curl_setopt($this->cURLHandle, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($this->cURLHandle, CURLOPT_TIMEOUT, 60);
    curl_setopt($this->cURLHandle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_setopt($this->cURLHandle, CURLOPT_HTTPHEADER, $headers);

    //assign soap XML 
    curl_setopt($this->cURLHandle, CURLOPT_POST, 1);
    curl_setopt($this->cURLHandle, CURLOPT_POSTFIELDS, $request);

    $response = curl_exec($this->cURLHandle);

    if (preg_match('/<CrmTicket>(.*)<\/CrmTicket>/', $response, $matches)) {
      $this->crmTicket = $matches[1];
      return true;
    } else {
      echo(curl_error($this->cURLHandle));
      throw new Exception('MSCrmIFD::getAccess() IFD auth failed');
    }
  }

  public function request($request, $action) {
    $headers = array(
      "POST " . $this->crmPath . " HTTP/1.1",
      "Host: " . $this->crmHost,
      'Connection: keep-alive',
      "SOAPAction: " . $action,
      "Content-type: text/xml;charset=utf-8",
      "Content-length: " . strlen($request),
    );

    // TODO: Check that crmpath has a / at the front

    curl_setopt($this->cURLHandle, CURLOPT_URL, $this->crmProto . "://" . $this->crmHost . $this->crmPath);
    curl_setopt($this->cURLHandle, CURLOPT_HTTPHEADER, $headers);
    curl_setopt($this->cURLHandle, CURLOPT_POST, 1);
    curl_setopt($this->cURLHandle, CURLOPT_POSTFIELDS, $request);

    // ticket is set into cookie, with that you dont need him in soap header anymore
    // in fact this row is most important in whole struggle with this-
    curl_setopt($this->cURLHandle, CURLOPT_COOKIE, 'MSCRMSession=ticket=' . $this->crmTicket . ';'); 

    $response = curl_exec($this->cURLHandle);            
    $responseHeaders = curl_getinfo($this->cURLHandle);

    if ($responseHeaders['http_code'] != 200) {
      print_r($response);
      die('MSCrmIFD::__doRequest() failed');
    }

    return $response;
  }    

  public function getAuthHeader() {
    $header = '
    <soap:Header>
      <CrmAuthenticationToken xmlns="http://schemas.microsoft.com/crm/2007/WebServices">
        <AuthenticationType xmlns="http://schemas.microsoft.com/crm/2007/CoreTypes">2</AuthenticationType>
        <OrganizationName xmlns="http://schemas.microsoft.com/crm/2007/CoreTypes">' . $this->org . '</OrganizationName>
      </CrmAuthenticationToken>
    </soap:Header>';

    return $header;
  }  

  public function closeConnection() {
    curl_close($this->cURLHandle);
  }          
}
?>