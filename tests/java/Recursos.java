import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.function.IntBinaryOperator;

/**
 * Classe de teste do leitor-exibidor (compilada com --release 8 -g).
 * Exercita estruturas que os exemplos da disciplina não cobrem: constantes
 * especiais (NaN, Infinity, long, strings com escapes e fora do BMP), lambdas
 * (invokedynamic + BootstrapMethods + MethodHandle/MethodType), genéricos
 * (Signature, LocalVariableTypeTable), exceções (Exceptions, exception table),
 * classes internas (InnerClasses, EnclosingMethod), StackMapTable,
 * multianewarray, instanceof/checkcast, monitorenter/monitorexit e switch de String.
 */
public class Recursos<T extends Comparable<T>> implements Runnable, java.io.Serializable {
    public static final float NAN_F = Float.NaN;
    public static final float INF_F = Float.POSITIVE_INFINITY;
    public static final double NEG_INF_D = Double.NEGATIVE_INFINITY;
    public static final double PEQUENO = 1.0e-10;
    public static final float UM_TERCO = 1.0f / 3.0f;
    public static final long GRANDE = 9_000_000_000L;
    public static final int INTEIRO = 123456;
    public static final String TEXTO = "linha1\nlinha2\ttab ação 😀 \u0000fim";
    public static final char LETRA = 'x';
    public static final boolean FLAG = true;

    private List<T> itens = new ArrayList<>();
    protected volatile int contador;
    transient Object cache;

    @Deprecated
    public void antigo() {
    }

    @Override
    public void run() {
        IntBinaryOperator soma = (a, b) -> a + b;
        Runnable r = () -> contador++;
        r.run();
        contador = soma.applyAsInt(contador, 2);
    }

    public synchronized void adicionar(T item) throws IOException, InterruptedException {
        if (item == null) {
            throw new IOException("nulo");
        }
        itens.add(item);
    }

    public int tentar(String s) {
        try {
            return Integer.parseInt(s);
        } catch (NumberFormatException e) {
            return -1;
        } finally {
            contador++;
        }
    }

    public int[][] matriz() {
        return new int[3][4];
    }

    public String texto(Object o) {
        if (o instanceof String) {
            return (String) o;
        }
        switch (String.valueOf(o)) {
            case "a": return "A";
            case "b": return "B";
            default: return "?";
        }
    }

    public long longos(long a, double d, float f) {
        long r = a * 2L + (long) d;
        r += (long) (f / 3.0f);
        return r >>> 1;
    }

    public void sincronizado() {
        synchronized (this) {
            contador--;
        }
    }

    class Interna {
        int valor() {
            return contador;
        }
    }

    static class Aninhada {
    }

    public Runnable anonima() {
        return new Runnable() {
            @Override
            public void run() {
                contador = 0;
            }
        };
    }
}
