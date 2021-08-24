import javax.swing.*;
import java.net.*;
import java.util.*;
import java.awt.event.*;
import java.io.*;
import java.awt.*;
import javax.swing.border.Border;
public class ChatClientGUI{

	Socket sock;
	DataOutputStream writer;
	InputStream reader; 
	String loginName;
	JFrame frame;
	JTextField text1;
	JTextField text2;
	JTextField text3;
	JButton login;
	JButton send;
	JButton logout;
	JTextArea incoming;
	DefaultListModel<String> model=new DefaultListModel<>();
	JList<String> userList;
	public static void main(String[] args){
		
		ChatClientGUI gui=new ChatClientGUI();
		gui.go();
		
	}
	public void go()
	{
		frame=new JFrame("ChatApp");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		
		JPanel panel=new JPanel();
		frame.setBackground(Color.darkGray);
		
		JLabel users=new JLabel("UsersList");
		JLabel message=new JLabel("Message");
		JLabel username=new JLabel("UserName");
		JLabel password=new JLabel("Password");
		text1=new JTextField();
		text2=new JTextField();
		text3=new JTextField();
		login=new JButton("Login");
		send=new JButton("Send");
		logout=new JButton("Logout");
		incoming=new JTextArea();
		userList=new JList<>(model);
		userList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		incoming.setEditable(false);
		incoming.setLineWrap(true);
		incoming.setWrapStyleWord(true);
		
		Border border=BorderFactory.createLineBorder(Color.BLACK);
		incoming.setBorder(border);
		userList.setBorder(border);
		
		
		
		
		text1.setBounds(100,40,250,25);
		text2.setBounds(450,40,250,25);
		login.setBounds(750,40,100,25);
		username.setBounds(20,40,250,20);
		password.setBounds(375,40,250,20);
		incoming.setBounds(40,110,600,400);
		message.setBounds(40,530,250,20);
		text3.setBounds(130,530,250,25);
		send.setBounds(400,530,100,25);
		logout.setBounds(500,570,100,25);
		userList.setBounds(670,110,150,500);
		users.setBounds(670,85,100,25);
		
		login.addActionListener(new LoginListener());
		logout.addActionListener(new LogoutListener());
		send.addActionListener(new SendListener());
		
		frame.add(text1);
		frame.add(text2);
		frame.add(login);
		frame.add(username);
		frame.add(password);
		frame.add(incoming);
		frame.add(text3);
		frame.add(message);
		frame.add(send);
		frame.add(logout);
		frame.add(userList);
		frame.add(users);
		setUpNetworking();
		Thread readerThread=new Thread(new IncomingReader());
		readerThread.start();
		
		frame.setSize(900,700);
		frame.setLayout(null);
		frame.setVisible(true);
		
			
		}
	
	
	
	
		private void setUpNetworking(){
			try
			{
				sock=new Socket("127.0.0.1",30000);
				reader=sock.getInputStream();
				writer=new DataOutputStream(sock.getOutputStream());
			}
			catch(IOException ex)
			{
				ex.printStackTrace();
			}
		}
	
		
		public class LoginListener implements ActionListener {
			public void actionPerformed(ActionEvent ev){
				
				try{
					 
					String username=text1.getText();
					String password=text2.getText();
					
					byte commandId=1;
					short datalen=(short)(2+username.length()+password.length());
					byte userlen=(byte)username.length();
					
					byte passlen=(byte)password.length();
					
					ByteArrayOutputStream buf=new ByteArrayOutputStream();
					DataOutputStream out=new DataOutputStream(buf);
					
					byte[] u=username.getBytes();
					byte[] p=password.getBytes();
					
					out.writeByte(commandId);
					out.writeShort(datalen);
					out.writeByte(userlen);
					out.write(u);
					out.writeByte(passlen);
					out.write(p);
					out.flush();
					byte[] msg=buf.toByteArray();
					writer.write(msg);
				   
					}
					catch(Exception ex)
					{
						ex.printStackTrace();
					}
			
			}
		}
		
		public void loginResponse(byte[] msg)
		{
			if(msg[1]==0)
			{
				text1.setEditable(false);
				text2.setEditable(false);
				JOptionPane.showMessageDialog(frame,"Login successful");
				loginName=text1.getText();
				System.out.println("i am "+loginName);
				
			}
			else
			{
				JOptionPane.showMessageDialog(frame,"Login failed");	
			}
			
		}
		
		public String getString(byte[] msg,int start,int end){
			String name="";
			for(int index=start;index<end;index++)
			{
				name+=(char)msg[index];
			}
			return name;
		}

		public void addUser(byte[] msg)
		{
			String name=getString(msg,3,msg.length);
			model.addElement(name);
			System.out.println("adding "+name+" to my side display");
			System.out.println(model.toString());
 			userList.setModel(model);
					
		
			
		}
		
		public void removeUser(byte[] msg)
		{
			String un=getString(msg,3,msg.length);
			for(int index=0;index<model.getSize();index++)
			{
				String name=model.getElementAt(index);
				if(name.equals(un))
				{
					model.remove(index);
					System.out.println("removing "+name+" to my side display");
					return;
				}
			}	
		}
		
		public void displayReceivedMessage(byte[] msg)
		{
			int sourceLength=msg[3];
			String sourceName=getString(msg,4,4+sourceLength);			
			int destinationLength=msg[4+sourceLength];
			int n=4+sourceLength+1+destinationLength;
			int messageLength=msg[n];
			String message=getString(msg,n+2,msg.length);
			incoming.append(sourceName+": "+message+"\n");
			System.out.println("displaying message from"+sourceName+":"+message);
		}
		
		
	public class IncomingReader implements Runnable {
		
		public void run()
		{	
	try{
		byte[] buff=new byte[1024];
		int k=-1;
		while((k=sock.getInputStream().read(buff,0,buff.length))>-1)
		{
			byte[] msg=new byte[k];
			System.arraycopy(buff,0,msg,0,k);
			System.out.println(msg[0]);
				switch(msg[0])
				{
				case 1:
					loginResponse(msg);
					break;
				case 2:
					displayReceivedMessage(msg);
					break;
				case 3:
					addUser(msg);
					break;
				case 5:
					removeUser(msg);
					break;
				}
			
		}	
		}
		catch(Exception ex)
		{
			ex.printStackTrace();
		}
		}
	}
	
	public class SendListener implements ActionListener{
		public void actionPerformed(ActionEvent e){
			try{
				if(userList.getSelectedValue()!=null)
				{
					
					String dest=userList.getSelectedValue();
					String message=text3.getText();
					incoming.append(loginName+": "+message+"\n");
					byte commandId=2;
					byte sourceLength=(byte)loginName.length();
					byte destinationLength=(byte)dest.length();
					short messageLength=(short)message.length();
					short totalMessageLength=(short)(4+sourceLength+destinationLength+messageLength);
					
					ByteArrayOutputStream buf=new ByteArrayOutputStream();
					DataOutputStream out=new DataOutputStream(buf);
					
					out.writeByte(commandId);
					out.writeShort(totalMessageLength);
					out.writeByte(sourceLength);
					out.write(loginName.getBytes());
					out.writeByte(destinationLength);
					out.write(dest.getBytes());
					out.writeShort(messageLength);
					out.write(message.getBytes());
					out.flush();
					byte[] msg=buf.toByteArray();
					writer.write(msg);
					text3.setText("");
					userList.clearSelection();
					
				}
				
				
			}
			catch(Exception ex){
				
				
			}
			
		}
	}
		
	public class LogoutListener implements ActionListener{
		
		public void actionPerformed(ActionEvent e){
			try{
			byte commandId=5;
			short datalen=(short)loginName.length();
			byte b[]=loginName.getBytes();
			ByteArrayOutputStream buf=new ByteArrayOutputStream();
			DataOutputStream out=new DataOutputStream(buf);
			out.writeByte(commandId);
			out.writeShort(datalen);
			out.write(b);
			out.flush();
			byte[] msg=buf.toByteArray();
			writer.write(msg);
			sock.close();
		frame.dispose();
			}
			catch(Exception ex)
			{
				ex.printStackTrace();
			}

		}}
		
}





