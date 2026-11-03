import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Classe de teste do leitor-exibidor (compilada com --release 17 -parameters).
 * Exercita atributos de versões mais novas do Java: PermittedSubclasses
 * (sealed), Record, NestHost/NestMembers, MethodParameters, anotações
 * (RuntimeVisibleAnnotations, RuntimeVisibleParameterAnnotations,
 * AnnotationDefault) e concatenação de strings via invokedynamic.
 */
public sealed interface Moderno permits Moderno.Ponto, Moderno.Vazio {

    @Retention(RetentionPolicy.RUNTIME)
    @interface Info {
        String autor() default "grupo";
        int versao() default 1;
        Cor cor() default Cor.AZUL;
        Class<?> tipo() default Object.class;
        String[] tags() default {};
    }

    enum Cor { AZUL, VERDE }

    @Info(autor = "SB", versao = 2, cor = Cor.VERDE, tipo = String.class, tags = {"a", "b"})
    record Ponto(int x, int y) implements Moderno {
        public String descrever(String prefixo, @Deprecated int casas) {
            return prefixo + "(" + x + ", " + y + ")";
        }
    }

    final class Vazio implements Moderno {
    }
}
