class S{static int n=0;public static void main(String[]a)throws Exception{var t=new Thread(()->{synchronized(S.class){n++;}});t.start();t.join();System.out.println(n);}}
